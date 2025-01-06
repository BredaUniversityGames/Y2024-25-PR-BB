#include "pipelines/ssao_pipeline.hpp"

#include "camera.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

#include "resource_management/buffer_resource_manager.hpp"
#include <random>
#include <resource_management/sampler_resource_manager.hpp>
#include <single_time_commands.hpp>

SSAOPipeline::SSAOPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const ResourceHandle<GPUImage>& ssaoTarget)
    : _pushConstants()
    , _context(context)
    , _gBuffers(gBuffers)
    , _ssaoTarget(ssaoTarget)
{
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();
    _pushConstants.positionIndex = _gBuffers.Attachments()[3].Index();

    vk::PhysicalDeviceProperties properties {};
    _context->VulkanContext()->PhysicalDevice().getProperties(&properties);

    CreateBuffers();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
    CreatePipeline();
}

void SSAOPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "SSAO Pipeline");

    _pushConstants.screenWidth = _gBuffers.Size().x;
    _pushConstants.screenHeight = _gBuffers.Size().y;

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
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _descriptorSet }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, scene.gpuScene->MainCamera().DescriptorSet(currentFrame), {});
    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

SSAOPipeline::~SSAOPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
    _context->VulkanContext()->Device().destroy(_descriptorSetLayout);
    _context->Resources()->BufferResourceManager().Destroy(_sampleKernelBuffer);
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

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetColorAttachmentFormats({ vk::Format::eR8Unorm })
                      .SetDepthAttachmentFormat(vk::Format::eUndefined)
                      .BuildPipeline();

    _pipelineLayout = std::get<0>(result);
    _pipeline = std::get<1>(result);
}
void SSAOPipeline::CreateBuffers()
{
    // c++ side first
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    std::vector<glm::vec4> ssaoKernel;
    for (uint32_t i = 0; i < 64; ++i)
    {
        glm::vec4 sample(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator),
            0.0f); // padding
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);

        // scale magic to be more cluster near the actual fragment.
        float scale = static_cast<float>(i) / 64.0;
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    std::vector<glm::vec4> ssaoNoise;
    for (uint32_t i = 0; i < 16; i++)
    {
        glm::vec4 noise(
            randomFloats(generator) * 2.0 - 1.0,
            randomFloats(generator) * 2.0 - 1.0,
            0.0f,
            0.0f); // padding
        ssaoNoise.push_back(noise);
    }

    auto resources { _context->Resources() };
    auto cmdBuffer = SingleTimeCommands(_context->VulkanContext());

    // Sample Kernel buffer
    {
        BufferCreation creation {};
        creation.SetName("Sample Kernel")
            .SetSize(ssaoKernel.size() * sizeof(glm::vec4))
            .SetIsMappable(false)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
            .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);

        _sampleKernelBuffer = resources->BufferResourceManager().Create(creation);
        cmdBuffer.CopyIntoLocalBuffer(ssaoKernel, 0, resources->BufferResourceManager().Access(_sampleKernelBuffer)->buffer);
    }

    std::vector<std::byte> byteData;
    byteData.reserve(ssaoNoise.size() * sizeof(float) * 4);

    for (const auto& color : ssaoNoise)
    {
        // No clamping, store raw floats (including negative)
        float components[4] = { color.r, color.g, color.b, color.a };

        // Push raw float bytes directly
        const std::byte* rawBytes = reinterpret_cast<const std::byte*>(components);
        byteData.insert(byteData.end(), rawBytes, rawBytes + sizeof(components));
    }

    // Use a float format
    CPUImage noiseImage {};
    noiseImage.SetName("SSAO_Noise_Image")
        .SetSize(4, 4, 1)
        .SetData(std::move(byteData))
        .SetFlags(vk::ImageUsageFlagBits::eSampled)
        .SetFormat(vk::Format::eR32G32B32A32Sfloat);
    noiseImage.isHDR = true;

    SamplerCreation noiseSampler {};
    noiseSampler.name = "FastNoiseSampler";
    noiseSampler.addressModeU = vk::SamplerAddressMode::eRepeat;
    noiseSampler.addressModeV = vk::SamplerAddressMode::eRepeat;
    noiseSampler.addressModeW = vk::SamplerAddressMode::eRepeat;

    noiseSampler.minFilter = vk::Filter::eNearest;
    noiseSampler.magFilter = vk::Filter::eNearest;
    noiseSampler.mipmapMode = vk::SamplerMipmapMode::eNearest;

    noiseSampler.useMaxAnisotropy = false;
    noiseSampler.anisotropyEnable = false;
    noiseSampler.minLod = 0.0f;
    noiseSampler.maxLod = 0.0f; // No mipmaps

    noiseSampler.compareEnable = false;
    noiseSampler.compareOp = vk::CompareOp::eAlways;
    noiseSampler.unnormalizedCoordinates = false;
    noiseSampler.borderColor = vk::BorderColor::eIntOpaqueBlack;

    _noiseSampler = _context->Resources()->SamplerResourceManager().Create(noiseSampler);
    _ssaoNoise = _context->Resources()->ImageResourceManager().Create(noiseImage, _noiseSampler);
    _pushConstants.ssaoNoiseIndex = _ssaoNoise.Index();
}
void SSAOPipeline::CreateDescriptorSetLayouts()
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings = {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
            .pImmutableSamplers = nullptr }
    };

    _descriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, { "uSampleKernel" });
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

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites = {
        vk::WriteDescriptorSet {
            .dstSet = _descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &sampleKernelBufferInfo }
    };

    _context->VulkanContext()->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
