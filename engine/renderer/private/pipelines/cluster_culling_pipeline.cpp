#include "pipelines/cluster_culling_pipeline.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

ClusterCullingPipeline::ClusterCullingPipeline(const std::shared_ptr<GraphicsContext>& context, GPUScene& gpuScene,
    ResourceHandle<Buffer>& clusterBuffer, ResourceHandle<Buffer>& globalIndex,
    ResourceHandle<Buffer>& lightCells, ResourceHandle<Buffer>& lightIndices)
    : _context(context)
    , _gpuScene(gpuScene)
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

    _context->VulkanContext()->Device().destroy(_cullingDescriptorSetLayout);
}

void ClusterCullingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    // commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    // commandBuffer.dispatch(16, 9, 24);
}

void ClusterCullingPipeline::CreatePipeline()
{
    std::array<vk::DescriptorSetLayout, 4> layouts {
        _gpuScene.GetClusterDescriptorSetLayout(),
        _gpuScene.GetPointLightDescriptorSetLayout(),
        _cullingDescriptorSetLayout,
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
    std::array<vk::DescriptorSetLayoutBinding, 3> layoutBinding;

    for (size_t i = 0; i < layoutBinding.size(); i++)
    {
        layoutBinding.at(i) = {
            .binding = static_cast<uint32_t>(i),
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        };
    }

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .bindingCount = 3,
        .pBindings = layoutBinding.data(),
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_cullingDescriptorSetLayout), "Failed to create descriptor set layout for cluster culling pipeline");

    vk::DescriptorSetLayout layout { _cullingDescriptorSetLayout };

    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, &_cullingDescriptorSet), "Failed to allocate descriptor set for cluster culling pipeline");

    std::array<vk::WriteDescriptorSet, 3> writeDescriptorSets;

    std::array buffers = {
        _context->Resources()->BufferResourceManager().Access(_globalIndex),
        _context->Resources()->BufferResourceManager().Access(_lightCells),
        _context->Resources()->BufferResourceManager().Access(_lightIndices),
    };

    for (size_t i = 0; i < buffers.size(); i++)
    {
        vk::DescriptorBufferInfo bufferInfo {
            .buffer = buffers.at(i)->buffer,
            .offset = 0,
            .range = vk::WholeSize,
        };

        writeDescriptorSets.at(i) = {
            .dstSet = _cullingDescriptorSet,
            .dstBinding = static_cast<uint32_t>(i),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &bufferInfo,
        };
    }

    _context->VulkanContext()->Device().updateDescriptorSets(1, writeDescriptorSets.data(), 0, nullptr);
}
