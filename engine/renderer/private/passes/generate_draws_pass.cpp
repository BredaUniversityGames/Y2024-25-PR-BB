#include "passes/generate_draws_pass.hpp"

#include "camera_batch.hpp"
#include "gpu_resources.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

GenerateDrawsPass::GenerateDrawsPass(const std::shared_ptr<GraphicsContext>& context, const CameraBatch& cameraBatch, const bool drawStatic, const bool drawDynamic)
    : _context(context)
    , _cameraBatch(cameraBatch)
{
    CreateCullingPipeline();
    _shouldDrawStatic = drawStatic;
    _shouldDrawDynamic = drawDynamic;
}

GenerateDrawsPass::~GenerateDrawsPass()
{
    _context->VulkanContext()->Device().destroy(_generateDrawsPipeline);
    _context->VulkanContext()->Device().destroy(_generateDrawsPipelineLayout);
}

void GenerateDrawsPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    const auto& imageResourceManager = _context->Resources()->ImageResourceManager();
    const auto& bufferResourceManager = _context->Resources()->BufferResourceManager();
    const auto* depthImage = imageResourceManager.Access(_cameraBatch.DepthImage());

    PushConstants staticPc {
        .isPrepass = _isPrepass,
        .mipSize = std::fmax(static_cast<float>(depthImage->width), static_cast<float>(depthImage->height)),
        .hzbIndex = _cameraBatch.HZBImage().Index(),
        .drawCommandsCount = scene.gpuScene->StaticDrawCount(),
        .isReverseZ = _cameraBatch.Camera().UsesReverseZ(),
        .drawStaticDraws = static_cast<uint32_t>(_shouldDrawStatic),
    };
    PushConstants skinnedPc = staticPc;
    skinnedPc.drawCommandsCount = scene.gpuScene->SkinnedDrawCount();
    skinnedPc.drawStaticDraws = static_cast<uint32_t>(_shouldDrawDynamic);

    std::array<vk::Buffer, 2> buffers;
    buffers[0] = bufferResourceManager.Access(_cameraBatch.StaticDraw().redirectBuffer)->buffer;
    buffers[1] = bufferResourceManager.Access(_cameraBatch.SkinnedDraw().redirectBuffer)->buffer;
    commandBuffer.fillBuffer(buffers[0], 0, sizeof(uint32_t), 0);
    commandBuffer.fillBuffer(buffers[1], 0, sizeof(uint32_t), 0);

    std::array<vk::BufferMemoryBarrier, 2> fillBarriers;
    fillBarriers[0] = {
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = buffers[0],
        .offset = 0,
        .size = sizeof(uint32_t),
    };
    fillBarriers[1] = fillBarriers[0];
    fillBarriers[1].buffer = buffers[1];

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, fillBarriers, {});

    if (_isPrepass)
    {
        TracyVkZone(scene.tracyContext, commandBuffer, "Prepass generate draws");

        RecordPrepassCommands(commandBuffer,
            currentFrame,
            _cameraBatch.StaticDraw(),
            scene.gpuScene->StaticDrawDescriptorSet(currentFrame),
            scene.gpuScene->GetStaticInstancesDescriptorSet(currentFrame),
            scene.gpuScene->StaticDrawCount(),
            staticPc);
        RecordPrepassCommands(commandBuffer,
            currentFrame,
            _cameraBatch.SkinnedDraw(),
            scene.gpuScene->SkinnedDrawDescriptorSet(currentFrame),
            scene.gpuScene->GetSkinnedInstancesDescriptorSet(currentFrame),
            scene.gpuScene->SkinnedDrawCount(),
            skinnedPc);
    }
    else
    {
        TracyVkZone(scene.tracyContext, commandBuffer, "Second pass generate draws");

        RecordSecondPassCommands(commandBuffer,
            currentFrame,
            _cameraBatch.StaticDraw(),
            scene.gpuScene->StaticDrawDescriptorSet(currentFrame),
            scene.gpuScene->GetStaticInstancesDescriptorSet(currentFrame),
            scene.gpuScene->StaticDrawCount(),
            staticPc);
        RecordSecondPassCommands(commandBuffer,
            currentFrame,
            _cameraBatch.SkinnedDraw(),
            scene.gpuScene->SkinnedDrawDescriptorSet(currentFrame),
            scene.gpuScene->GetSkinnedInstancesDescriptorSet(currentFrame),
            scene.gpuScene->SkinnedDrawCount(),
            skinnedPc);
    }

    _isPrepass = !_isPrepass;
}

void GenerateDrawsPass::RecordPrepassCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const CameraBatch::Draw& draw, vk::DescriptorSet sceneDraws, vk::DescriptorSet sceneInstances, uint32_t drawCount, const PushConstants& pc)
{
    const auto& bufferResourceManager = _context->Resources()->BufferResourceManager();

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _generateDrawsPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 1, { sceneDraws }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 2, { draw.drawDescriptor }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 3, { sceneInstances }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 4, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 5, { draw.visibilityDescriptor }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 6, { draw.redirectDescriptor }, {});

    commandBuffer.pushConstants<PushConstants>(_generateDrawsPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pc });

    commandBuffer.dispatch((drawCount + _localComputeSize - 1) / _localComputeSize, 1, 1);

    std::array<vk::BufferMemoryBarrier, 1> barriers;

    barriers[0] = {
        .srcAccessMask = vk::AccessFlagBits::eShaderRead,
        .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = bufferResourceManager.Access(draw.visibilityBuffer)->buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers[0], {});
}

void GenerateDrawsPass::RecordSecondPassCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const CameraBatch::Draw& draw, vk::DescriptorSet sceneDraws, vk::DescriptorSet sceneInstances, uint32_t drawCount, const PushConstants& pc)
{
    const auto& bufferResourceManager = _context->Resources()->BufferResourceManager();

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _generateDrawsPipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 1, { sceneDraws }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 2, { draw.drawDescriptor }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 3, { sceneInstances }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 4, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 5, { draw.visibilityDescriptor }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _generateDrawsPipelineLayout, 6, { draw.redirectDescriptor }, {});

    commandBuffer.pushConstants<PushConstants>(_generateDrawsPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { pc });

    commandBuffer.dispatch((drawCount + _localComputeSize - 1) / _localComputeSize, 1, 1);

    std::array<vk::BufferMemoryBarrier, 1> barriers;

    barriers[0] = {
        .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
        .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .buffer = bufferResourceManager.Access(draw.visibilityBuffer)->buffer,
        .offset = 0,
        .size = vk::WholeSize,
    };

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect, {}, {}, barriers[0], {});
}

void GenerateDrawsPass::CreateCullingPipeline()
{
    std::vector<std::byte> spvBytes = shader::ReadFile("shaders/bin/generate_draws.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, spvBytes);
    auto result = pipelineBuilder.BuildPipeline();

    _generateDrawsPipelineLayout = std::get<0>(result);
    _generateDrawsPipeline = std::get<1>(result);
}
