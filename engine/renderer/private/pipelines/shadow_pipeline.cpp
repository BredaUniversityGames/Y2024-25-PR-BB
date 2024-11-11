#include "pipelines/shadow_pipeline.hpp"

#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

#include <vector>

ShadowPipeline::ShadowPipeline(const std::shared_ptr<VulkanContext>& context, const GBuffers& gBuffers, const GPUScene& gpuScene)
    : _context(context)
    , _gBuffers(gBuffers)
    , _shadowCamera(_context)
    , _culler(_context, gpuScene)
{
    CreatePipeline();

    ResourceHandle<Buffer> mainDrawBufferHandle = gpuScene.IndirectDrawBuffer(0);
    const Buffer* mainDrawBuffer = _context->GetBufferResourceManager().Access(mainDrawBufferHandle);

    BufferCreation creation {
        .size = mainDrawBuffer->size,
        .usage = mainDrawBuffer->usage,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = "Shadow draw buffer",
    };

    _drawBuffer = _context->GetBufferResourceManager().Create(creation);

    CreateDrawBufferDescriptorSet(gpuScene);
}

ShadowPipeline::~ShadowPipeline()
{
    _context->Device().destroy(_pipeline);
    _context->Device().destroy(_pipelineLayout);

    _context->GetBufferResourceManager().Destroy(_drawBuffer);
}

void ShadowPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    _shadowCamera.Update(currentFrame, scene.sceneDescription->directionalLight.camera);

    _culler.RecordCommands(commandBuffer, currentFrame, scene, _shadowCamera, _drawBuffer, _drawBufferDescriptorSet);

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {
        .imageView = _context->GetImageResourceManager().Access(_gBuffers.Shadow())->view,
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 } },
    };

    vk::RenderingInfoKHR renderingInfo {
        .renderArea = {
            .extent = vk::Extent2D { 4096, 4096 } // TODO: Remove hard coded size
        },
        .layerCount = 1,
        .pDepthAttachment = &depthAttachmentInfo,
    };

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { scene.gpuScene->GetSceneDescriptorSet(currentFrame) }, {});

    vk::Buffer vertexBuffer = _context->GetBufferResourceManager().Access(scene.batchBuffer->VertexBuffer())->buffer;
    vk::Buffer indexBuffer = _context->GetBufferResourceManager().Access(scene.batchBuffer->IndexBuffer())->buffer;
    vk::Buffer indirectDrawBuffer = _context->GetBufferResourceManager().Access(_drawBuffer)->buffer;
    vk::Buffer indirectCountBuffer = _context->GetBufferResourceManager().Access(scene.gpuScene->IndirectCountBuffer(currentFrame))->buffer;

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.batchBuffer->IndexType());

    uint32_t indirectCountOffset = scene.gpuScene->IndirectCountOffset();
    commandBuffer.drawIndexedIndirectCountKHR(indirectDrawBuffer, 0, indirectCountBuffer, indirectCountOffset, scene.gpuScene->DrawCount(), sizeof(vk::DrawIndexedIndirectCommand), _context->Dldi());

    _context->GetDrawStats().IndirectDraw(scene.gpuScene->DrawCount(), scene.gpuScene->DrawCommandIndexCount());

    commandBuffer.endRenderingKHR(_context->Dldi());
}

void ShadowPipeline::CreatePipeline()
{
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/shadow.vert.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .SetColorBlendState(vk::PipelineColorBlendStateCreateInfo {})
        .SetDepthStencilState(depthStencilStateCreateInfo)
        .SetColorAttachmentFormats({})
        .SetDepthAttachmentFormat(_context->GetImageResourceManager().Access(_gBuffers.Shadow())->format)
        .BuildPipeline(_pipeline, _pipelineLayout);
}

void ShadowPipeline::CreateDrawBufferDescriptorSet(const GPUScene& gpuScene)
{
    vk::DescriptorSetLayout layout = gpuScene.DrawBufferLayout();
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    util::VK_ASSERT(_context->Device().allocateDescriptorSets(&allocateInfo, &_drawBufferDescriptorSet),
        "Failed allocating descriptor sets!");

    const Buffer* buffer = _context->GetBufferResourceManager().Access(_drawBuffer);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _drawBufferDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->Device().updateDescriptorSets({ bufferWrite }, {});
}
