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
    CreatePipeline();
}

ClusterCullingPipeline::~ClusterCullingPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void ClusterCullingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, { _gpuScene.GetClusterDescriptorSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 1, { _gpuScene.GetPointLightDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 2, { _gpuScene.GetClusterCullingDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 3, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});

    commandBuffer.dispatch(16, 9, 24);

    /*std::array<vk::BufferMemoryBarrier, 2> barriers;
    barriers[0] = {
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = _context->Resources()->BufferResourceManager().Access(_gpuScene.GetClusterCullingBuffer(0))->buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };

    barriers[1] = {
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = _context->Resources()->BufferResourceManager().Access(_gpuScene.GetClusterCullingBuffer(1))->buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {}, barriers, {});*/
}

void ClusterCullingPipeline::CreatePipeline()
{
    std::array<vk::DescriptorSetLayout, 4> layouts {
        _gpuScene.GetClusterDescriptorSetLayout(),
        _gpuScene.GetPointLightDescriptorSetLayout(),
        _gpuScene.GetClusterCullingDescriptorSetLayout(),
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
