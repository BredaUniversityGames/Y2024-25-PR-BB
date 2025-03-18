#include "passes/decals_pass.hpp"

#include "graphics_context.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

DecalsPass::DecalsPass(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers)
    : _context(context)
    , _gBuffers(gBuffers)
{
    CreateDescriptorSetLayouts();
    CreateBuffers();
    CreateDescriptorSets();
    CreatePipeline();
}

DecalsPass::~DecalsPass()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    vkContext->Device().destroy(_pipeline);
    vkContext->Device().destroy(_pipelineLayout);

    resources->BufferResourceManager().Destroy(_decalsBuffer);

    vkContext->Device().destroy(_decalsBufferDescriptorSetLayout);
}

void DecalsPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    UpdateDecals();

    TracyVkZone(scene.tracyContext, commandBuffer, "Decals Pass");

    // TODO: re-evaluate if it should be a rasterization pass or compute

    auto vkContext { _context->VulkanContext() };

    util::BeginLabel(commandBuffer, "Decals pass", glm::vec3 { 230.0f, 230.0f, 250.0f } / 255.0f, vkContext->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, _context->BindlessSet(), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 1, _decalsBufferDescriptorSet, {});

    _decalsPushConstant.decalCount = _decalCount;
    commandBuffer.pushConstants<DecalsPushConstant>(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { _decalsPushConstant });

    commandBuffer.dispatch(MAX_DECALS / 16, 1, 1);

    util::EndLabel(commandBuffer, vkContext->Dldi());
}

void DecalsPass::UpdateDecals()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    if (!_newDecals.empty())

        auto cmdBuffer = SingleTimeCommands(_context->VulkanContext());

    if (_decalOffset + _newDecals.size() > MAX_DECALS)
    {
        uint32_t newOffset = _decalOffset + _newDecals.size() - MAX_DECALS;
        std::vector<DecalData> splitLo(_newDecals.begin(), _newDecals.begin() +);
    }
    else
    {
        const Buffer* buffer = resources->BufferResourceManager().Access(_decalsBuffer);
        cmdBuffer.CopyIntoLocalBuffer(_newDecals, _decalOffset, buffer->buffer);

        _decalCount += _newDecals.size();
        _decalOffset += _newDecals.size();
    }

    cmdBuffer.Submit();
    _newDecals.clear();

    // TODO: buffer synchronization barrier to compute
}
}

void DecalsPass::CreatePipeline()
{
    auto vkContext { _context->VulkanContext() };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 3> layouts = { _context->BindlessLayout(), _decalsBufferDescriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    vk::PushConstantRange pcRange = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(_decalsPushConstant),
    };

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

    _pipelineLayout = vkContext->Device().createPipelineLayout(pipelineLayoutCreateInfo);

    std::vector<std::byte> byteCode = shader::ReadFile("shaders/bin/decals.comp.spv");

    vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, vkContext->Device());

    vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = shaderModule,
        .pName = "main",
    };

    vk::ComputePipelineCreateInfo computePipelineCreateInfo {
        .stage = shaderStageCreateInfo,
        .layout = _pipelineLayout,
    };

    auto result = vkContext->Device().createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the decals compute pipeline!");
    _pipeline = result.value;

    vkContext->Device().destroy(shaderModule);
}

void DecalsPass::CreateDescriptorSetLayouts()
{
    auto vkContext { _context->VulkanContext() };

    std::array<vk::DescriptorSetLayoutBinding, 1> bindings {};

    vk::DescriptorSetLayoutBinding& descriptorSetLayoutBinding { bindings[0] };
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
    descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo createInfo {};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    util::VK_ASSERT(vkContext->Device().createDescriptorSetLayout(&createInfo, nullptr, &_decalsBufferDescriptorSetLayout),
        "Failed creating emitter buffer descriptor set layout!");
}

void DecalsPass::CreateDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };

    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = vkContext->DescriptorPool();
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_decalsBufferDescriptorSetLayout;

    std::array<vk::DescriptorSet, 1> descriptorSets;
    util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating Emitter Uniform Buffer descriptor sets!");

    _decalsBufferDescriptorSet = descriptorSets[0];
    UpdateBuffersDescriptorSets();
}

void DecalsPass::UpdateBuffersDescriptorSets()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    std::array<vk::WriteDescriptorSet, 1> descriptorWrites {};

    vk::DescriptorBufferInfo decalsBufferInfo {};
    decalsBufferInfo.buffer = resources->BufferResourceManager().Access(_decalsBuffer)->buffer;
    decalsBufferInfo.offset = 0;
    decalsBufferInfo.range = sizeof(DecalData) * MAX_DECALS;
    vk::WriteDescriptorSet& decalsBufferWrite { descriptorWrites[0] };
    decalsBufferWrite.dstSet = _decalsBufferDescriptorSet;
    decalsBufferWrite.dstBinding = 0;
    decalsBufferWrite.dstArrayElement = 0;
    decalsBufferWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
    decalsBufferWrite.descriptorCount = 1;
    decalsBufferWrite.pBufferInfo = &decalsBufferInfo;

    vkContext->Device().updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void DecalsPass::CreateBuffers()
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    vk::DeviceSize bufferSize = sizeof(DecalData) * MAX_DECALS;
    BufferCreation creation {};
    creation.SetName("Decals UB")
        .SetSize(bufferSize)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetUsageFlags(vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst);
    _decalsBuffer = resources->BufferResourceManager().Create(creation);
}
