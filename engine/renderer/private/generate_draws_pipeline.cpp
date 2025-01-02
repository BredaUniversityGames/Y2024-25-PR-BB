#include "generate_draws_pipeline.hpp"

#include "camera_batch.hpp"
#include "gpu_resources.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

GenerateDrawsPipeline::GenerateDrawsPipeline(const std::shared_ptr<GraphicsContext>& context, const CameraBatch& cameraBatch)
    : _context(context)
    , _cameraBatch(cameraBatch)
{
    CreateCullingPipeline();
}

GenerateDrawsPipeline::~GenerateDrawsPipeline()
{
    _context->VulkanContext()->Device().destroy(_cullingPipeline);
    _context->VulkanContext()->Device().destroy(_cullingPipelineLayout);
}

void GenerateDrawsPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    static bool isPrepass = true;

    const auto& imageResourceManager = _context->Resources()->ImageResourceManager();
    const auto* depthImage = imageResourceManager.Access(_cameraBatch.DepthImage());

    const uint32_t localSize = 64;
    PushConstants pc {
        .isPrepass = isPrepass,
        .mipSize = std::fmax(static_cast<float>(depthImage->width), static_cast<float>(depthImage->height)),
        .hzbIndex = _cameraBatch.HZBImage().Index(),
    };
    if (isPrepass)
    {
        TracyVkZone(scene.tracyContext, commandBuffer, "Prepass cull");
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _cullingPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 1, { scene.gpuScene->DrawBufferDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 2, { _cameraBatch.DrawBufferDescriptorSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 3, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 4, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 5, { _cameraBatch.VisibilityBufferDescriptorSet() }, {});

        commandBuffer.pushConstants<PushConstants>(_cullingPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pc });

        commandBuffer.dispatch((scene.gpuScene->DrawCount() + localSize - 1) / localSize, 1, 1);

        vk::Buffer inDrawBuffer = _context->Resources()->BufferResourceManager().Access(scene.gpuScene->IndirectDrawBuffer(currentFrame))->buffer;
        vk::Buffer outDrawBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.DrawBuffer())->buffer;
        vk::Buffer visibilityBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.VisibilityBuffer())->buffer;

        std::array<vk::BufferMemoryBarrier, 3> barriers;

        barriers[0] = {
            .srcAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = inDrawBuffer,
            .offset = 0,
            .size = vk::WholeSize,
        };

        barriers[1] = barriers[0];
        barriers[1].buffer = outDrawBuffer;

        barriers[2] = barriers[0];
        barriers[2].buffer = visibilityBuffer;
        // barriers[2].srcAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers, {});
    }
    else
    {
        TracyVkZone(scene.tracyContext, commandBuffer, "Second pass cull");

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _cullingPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 1, { _cameraBatch.DrawBufferDescriptorSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 2, { _cameraBatch.DrawBufferDescriptorSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 3, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 4, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _cullingPipelineLayout, 5, { _cameraBatch.VisibilityBufferDescriptorSet() }, {});

        commandBuffer.pushConstants<PushConstants>(_cullingPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pc });

        commandBuffer.dispatch((scene.gpuScene->DrawCount() + localSize - 1) / localSize, 1, 1);

        vk::Buffer drawBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.DrawBuffer())->buffer;
        vk::Buffer visibilityBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.VisibilityBuffer())->buffer;

        std::array<vk::BufferMemoryBarrier, 2> barriers;

        barriers[0] = {
            .srcAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = drawBuffer,
            .offset = 0,
            .size = vk::WholeSize,
        };

        barriers[1] = barriers[0];
        barriers[1].buffer = visibilityBuffer;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers, {});
    }

    isPrepass = !isPrepass;
}

void GenerateDrawsPipeline::CreateCullingPipeline()
{
    std::vector<std::byte> spvBytes = shader::ReadFile("shaders/bin/culling.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, spvBytes);
    auto result = pipelineBuilder.BuildPipeline();

    _cullingPipelineLayout = std::get<0>(result);
    _cullingPipeline = std::get<1>(result);
}
