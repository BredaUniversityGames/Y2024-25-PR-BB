﻿#include "pipelines/ssao_pipeline.hpp"

#include "../vulkan_helper.hpp"
// #include "bloom_settings.hpp"
#include "camera.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

#include <random>
#include <resource_management/buffer_resource_manager.hpp>
#include <single_time_commands.hpp>

SSAOPipeline::SSAOPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& ssaoTarget)
    : _pushConstants()
    , _context(context)
    , _gBuffers(gBuffers)
    , _ssaoTarget(ssaoTarget)
{
    _pushConstants.albedoMIndex = _gBuffers.Attachments()[0].Index();
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();
    _pushConstants.emissiveAOIndex = _gBuffers.Attachments()[2].Index();
    _pushConstants.positionIndex = _gBuffers.Attachments()[3].Index();

    vk::PhysicalDeviceProperties properties {};
    _context->VulkanContext()->PhysicalDevice().getProperties(&properties);

    CreateBuffers();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
    CreatePipeline();
}

void SSAOPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) // NOLINT(*-include-cleaner)
{

    vk::RenderingAttachmentInfoKHR ssaoColorAttachmentInfo {
        .imageView = _context->Resources()->ImageResourceManager().Access(_ssaoTarget)->view,
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = { .color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } } }
    };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { _gBuffers.Size().x, _gBuffers.Size().y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &ssaoColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, _pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    // To add the descirpot sets for the kernel and noise buffers

    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

SSAOPipeline::~SSAOPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void SSAOPipeline::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/ssao.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats({ vk::Format::eR16Sfloat })
        .SetDepthAttachmentFormat(vk::Format::eUndefined)
        .BuildPipeline(_pipeline, _pipelineLayout);
}
void SSAOPipeline::CreateBuffers()
{
    // c++ side first
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // scale magic to be more cluster near the actual fragment.
        float scale = (float)i / 64.0;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    auto resources { _context->Resources() };
    auto cmdBuffer = SingleTimeCommands(_context->VulkanContext());

    // Sample Kernel buffer
    {
        BufferCreation creation {};
        creation.SetName("Sample Kernel")
            .SetSize(ssaoKernel.size() * sizeof(glm::vec3))
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

        _sampleKernelBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(ssaoKernel, 0, resources->BufferResourceManager().Access(_sampleKernelBuffer)->buffer);
    }

    // Random noise buffer
    {
        BufferCreation creation {};
        creation.SetName("Noise Kernel")
            .SetSize(ssaoNoise.size() * sizeof(glm::vec3))
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE)
            .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst);

        _noiseBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(ssaoNoise, 0, resources->BufferResourceManager().Access(_sampleKernelBuffer)->buffer);
    }
}
void SSAOPipeline::CreateDescriptorSetLayouts()
{
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr },
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr }
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    if (_context->VulkanContext()->Device().createDescriptorSetLayout(&layoutInfo, nullptr, &_descriptorSetLayout) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to create descriptor set layout");
    }
}
void SSAOPipeline::CreateDescriptorSets()
{
    vk::DescriptorSetAllocateInfo allocInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &_descriptorSetLayout
    };

    if (_context->VulkanContext()->Device().allocateDescriptorSets(&allocInfo, &_descriptorSet) != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to allocate descriptor set");
    }

    vk::DescriptorBufferInfo sampleKernelBufferInfo {
        .buffer = _context->Resources()->BufferResourceManager().Access(_sampleKernelBuffer)->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::DescriptorBufferInfo noiseBufferInfo {
        .buffer = _context->Resources()->BufferResourceManager().Access(_noiseBuffer)->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
        vk::WriteDescriptorSet {
            .dstSet = _descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &sampleKernelBufferInfo },
        vk::WriteDescriptorSet {
            .dstSet = _descriptorSet,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &noiseBufferInfo }
    };

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
