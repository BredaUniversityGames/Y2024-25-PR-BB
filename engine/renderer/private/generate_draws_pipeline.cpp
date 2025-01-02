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
    _context->VulkanContext()->Device().destroy(_generateDrawsPipeline);
    _context->VulkanContext()->Device().destroy(_generateDrawsPipelineLayout);
}

void GenerateDrawsPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    const auto& imageResourceManager = _context->Resources()->ImageResourceManager();
    const auto* depthImage = imageResourceManager.Access(_cameraBatch.DepthImage());

    const uint32_t localSize = 64;
    PushConstants pc {
        .isPrepass = _isPrepass,
        .mipSize = std::fmax(static_cast<float>(depthImage->width), static_cast<float>(depthImage->height)),
        .hzbIndex = _cameraBatch.HZBImage().Index(),
    };
    if (_isPrepass)
    {
        TracyVkZone(scene.tracyContext, commandBuffer, "Prepass generate draws");
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _generateDrawsPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 1, { scene.gpuScene->DrawBufferDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 2, { _cameraBatch.DrawBufferDescriptorSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 3, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 4, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 5, { _cameraBatch.VisibilityBufferDescriptorSet() }, {});

        commandBuffer.pushConstants<PushConstants>(_generateDrawsPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pc });

        commandBuffer.dispatch((scene.gpuScene->DrawCount() + localSize - 1) / localSize, 1, 1);

        vk::Buffer visibilityBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.VisibilityBuffer())->buffer;

        std::array<vk::BufferMemoryBarrier, 1> barriers;

        barriers[0] = {
            .srcAccessMask = vk::AccessFlagBits::eShaderRead,
            .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = visibilityBuffer,
            .offset = 0,
            .size = vk::WholeSize,
        };

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers, {});
    }
    else
    {
        TracyVkZone(scene.tracyContext, commandBuffer, "Second pass generate draws");

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _generateDrawsPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 1, { _cameraBatch.DrawBufferDescriptorSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 2, { _cameraBatch.DrawBufferDescriptorSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 3, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 4, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 5, { _cameraBatch.VisibilityBufferDescriptorSet() }, {});

        commandBuffer.pushConstants<PushConstants>(_generateDrawsPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pc });

        commandBuffer.dispatch((scene.gpuScene->DrawCount() + localSize - 1) / localSize, 1, 1);

        vk::Buffer visibilityBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.VisibilityBuffer())->buffer;

        std::array<vk::BufferMemoryBarrier, 1> barriers;

        barriers[0] = {
            .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .buffer = visibilityBuffer,
            .offset = 0,
            .size = vk::WholeSize,
        };

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers, {});
    }

    _isPrepass = !_isPrepass;
}

void GenerateDrawsPipeline::CreateCullingPipeline()
{
    std::vector<std::byte> spvBytes = shader::ReadFile("shaders/bin/culling.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, spvBytes);
    auto result = pipelineBuilder.BuildPipeline();

    _generateDrawsPipelineLayout = std::get<0>(result);
    _generateDrawsPipeline = std::get<1>(result);
}
