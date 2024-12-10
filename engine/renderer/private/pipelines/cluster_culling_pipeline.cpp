#include "pipelines/cluster_culling_pipeline.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

ClusterCullingPipeline::ClusterCullingPipeline(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain,
    ResourceHandle<Buffer>& clusterBuffer, ResourceHandle<Buffer>& globalIndex,
    ResourceHandle<Buffer>& lightCells, ResourceHandle<Buffer>& lightIndices)
    : _context(context)
    , _swapChain(swapChain)
    , _clusterBuffer(clusterBuffer)
    , _globalIndex(globalIndex)
    , _lightCells(lightCells)
    , _lightIndices(lightIndices)
{
    CreateDescriptorSet();
    CreatePipeline();
}

ClusterCullingPipeline::~ClusterCullingPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);

    _context->VulkanContext()->Device().destroy(_clusterInputDescriptorSetLayout);
    _context->VulkanContext()->Device().destroy(_globalIndexDescriptorSetLayout);
    _context->VulkanContext()->Device().destroy(_lightCellsDescriptorSetLayout);
    _context->VulkanContext()->Device().destroy(_lightIndicesDescriptorSetLayout);
}

void ClusterCullingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    // commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    // commandBuffer.dispatch(16, 9, 24);
}

void ClusterCullingPipeline::CreatePipeline()
{
    std::array<vk::DescriptorSetLayout, 6> layouts {
        _clusterInputDescriptorSetLayout,
        _globalIndexDescriptorSetLayout,
        _lightCellsDescriptorSetLayout,
        _lightIndicesDescriptorSetLayout,
        CameraResource::DescriptorSetLayout(),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout), "Failed to create pipeline layout for cluster culling pipeline");

    std::vector<std::byte> compShader = shader::ReadFile("shaders/bin/cluster_light_culling.comp.spv");
    vk::ShaderModule computeModule = shader::CreateShaderModule(compShader, _context->VulkanContext()->Device());

    vk::PipelineShaderStageCreateInfo computeStage {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = computeModule,
        .pName = "main",
    };

    vk::ComputePipelineCreateInfo pipelineCreateInfo {
        .stage = computeStage,
        .layout = _pipelineLayout,
    };

    auto result = _context->VulkanContext()->Device().createComputePipeline(nullptr, pipelineCreateInfo, nullptr);
    _pipeline = result.value;
    _context->VulkanContext()->Device().destroy(computeModule);
}

void ClusterCullingPipeline::CreateDescriptorSet()
{
    vk::DescriptorSetLayoutBinding layoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
    };+

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .bindingCount = 1,
        .pBindings = &layoutBinding,
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_clusterInputDescriptorSetLayout), "Failed to create descriptor set layout for cluster culling pipeline");
    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_globalIndexDescriptorSetLayout), "Failed to create descriptor set layout for cluster culling pipeline");
    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_lightCellsDescriptorSetLayout), "Failed to create descriptor set layout for cluster culling pipeline");
    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_lightIndicesDescriptorSetLayout), "Failed to create descriptor set layout for cluster culling pipeline");

    std::array<vk::DescriptorSetLayout, 6> layouts {
        _clusterInputDescriptorSetLayout,
        _globalIndexDescriptorSetLayout,
        _lightCellsDescriptorSetLayout,
        _lightIndicesDescriptorSetLayout,
        CameraResource::DescriptorSetLayout(),

    };

    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 6,
        .pSetLayouts = layouts.data(),
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, &_baseDecsriptorSet), "Failed to allocate descriptor set for cluster culling pipeline");

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_lightCells);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _baseDecsriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets(1, &bufferWrite, 0, nullptr);
}
