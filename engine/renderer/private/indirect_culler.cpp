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

IndirectCuller::IndirectCuller(const std::shared_ptr<GraphicsContext>& context, const CameraResource& camera, ResourceHandle<Buffer> targetBuffer, vk::DescriptorSet targetDescriptorSet, ResourceHandle<Buffer> visibilityBuffer, vk::DescriptorSet visibilityDescriptorSet)
    : _context(context)
    , _camera(camera)
    , _targetBuffer(targetBuffer)
    , _targetDescriptorSet(targetDescriptorSet)
    , _visibilityBuffer(visibilityBuffer)
    , _visibilityDescriptorSet(visibilityDescriptorSet)
{
    CreateCullingPipeline();
}

IndirectCuller::~IndirectCuller()
{
    _context->VulkanContext()->Device().destroy(_cullingPipeline);
    _context->VulkanContext()->Device().destroy(_cullingPipelineLayout);
}

void IndirectCuller::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool isPrepass)
{
    if (isPrepass)
    {
        const uint32_t localSize = 64;
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _cullingPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 0, { scene.gpuScene->DrawBufferDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 1, { _targetDescriptorSet }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 2, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 3, { _camera.DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 4, { _visibilityDescriptorSet }, {});

        commandBuffer.pushConstants<uint32_t>(_cullingPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { isPrepass });

        commandBuffer.dispatch((scene.gpuScene->DrawCount() + localSize - 1) / localSize, 1, 1);

        vk::Buffer inDrawBuffer = _context->Resources()->BufferResourceManager().Access(scene.gpuScene->IndirectDrawBuffer(currentFrame))->buffer;
        vk::Buffer outDrawBuffer = _context->Resources()->BufferResourceManager().Access(_targetBuffer)->buffer;

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
    else
    {
    }
}

void IndirectCuller::CreateCullingPipeline()
{
    std::vector<std::byte> spvBytes = shader::ReadFile("shaders/bin/culling.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, spvBytes);
    auto result = pipelineBuilder.BuildPipeline();

    _cullingPipelineLayout = std::get<0>(result);
    _cullingPipeline = std::get<1>(result);
}
