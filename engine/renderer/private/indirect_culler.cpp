#include "indirect_culler.hpp"

#include "gpu_resources.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

IndirectCuller::IndirectCuller(const std::shared_ptr<GraphicsContext>& context, const GPUScene& gpuScene)
    : _context(context)
{
    CreateCullingPipeline(gpuScene);
}

IndirectCuller::~IndirectCuller()
{
    _context->VulkanContext()->Device().destroy(_cullingPipeline);
    _context->VulkanContext()->Device().destroy(_cullingPipelineLayout);
}

void IndirectCuller::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, const CameraResource& camera, ResourceHandle<Buffer> targetBuffer, vk::DescriptorSet targetDescriptorSet)
{
    const uint32_t localSize = 64;
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _cullingPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 0, { scene.gpuScene->DrawBufferDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 1, { targetDescriptorSet }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 2, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 3, { camera.DescriptorSet(currentFrame) }, {});
    commandBuffer.dispatch((scene.gpuScene->DrawCount() + localSize - 1) / localSize, 1, 1);

    vk::Buffer inDrawBuffer = _context->Resources()->BufferResourceManager().Access(scene.gpuScene->IndirectDrawBuffer(currentFrame))->buffer;
    vk::Buffer outDrawBuffer = _context->Resources()->BufferResourceManager().Access(targetBuffer)->buffer;

    std::array<vk::BufferMemoryBarrier, 2> barriers;

    barriers[0] = {
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = inDrawBuffer,
        .offset = 0,
        .size = vk::WholeSize,
    };

    barriers[1] = barriers[0];
    barriers[1].buffer = outDrawBuffer;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers, {});
}

void IndirectCuller::CreateCullingPipeline(const GPUScene& gpuScene)
{
    std::vector<std::byte> spvBytes = shader::ReadFile("shaders/bin/culling.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, spvBytes);
    auto result = pipelineBuilder.BuildPipeline();

    _cullingPipelineLayout = std::get<0>(result);
    _cullingPipeline = std::get<1>(result);
}
