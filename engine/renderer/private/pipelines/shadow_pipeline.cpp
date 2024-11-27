#include "pipelines/shadow_pipeline.hpp"

#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <vector>

ShadowPipeline::ShadowPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const GPUScene& gpuScene)
    : _context(context)
    , _gBuffers(gBuffers)
    , _shadowCamera(_context)
    , _culler(_context, gpuScene)
{
    CreatePipeline();

    ResourceHandle<Buffer> mainDrawBufferHandle = gpuScene.IndirectDrawBuffer(0);
    const Buffer* mainDrawBuffer = _context->Resources()->BufferResourceManager().Access(mainDrawBufferHandle);

    BufferCreation creation {
        .size = mainDrawBuffer->size,
        .usage = mainDrawBuffer->usage,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = "Shadow draw buffer",
    };

    _drawBuffer = _context->Resources()->BufferResourceManager().Create(creation);

    CreateDrawBufferDescriptorSet(gpuScene);
}

ShadowPipeline::~ShadowPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void ShadowPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    _shadowCamera.Update(currentFrame, scene.gpuScene->DirectionalLightShadowCamera());

    _culler.RecordCommands(commandBuffer, currentFrame, scene, _shadowCamera, _drawBuffer, _drawBufferDescriptorSet);

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {
        .imageView = resources->ImageResourceManager().Access(_gBuffers.Shadow())->view,
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

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { scene.gpuScene->GetSceneDescriptorSet(currentFrame) }, {});

    vk::Buffer vertexBuffer = resources->BufferResourceManager().Access(scene.staticBatchBuffer->VertexBuffer())->buffer;
    vk::Buffer indexBuffer = resources->BufferResourceManager().Access(scene.staticBatchBuffer->IndexBuffer())->buffer;
    vk::Buffer indirectDrawBuffer = resources->BufferResourceManager().Access(_drawBuffer)->buffer;

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.staticBatchBuffer->IndexType());

    commandBuffer.drawIndexedIndirect(
        indirectDrawBuffer,
        scene.gpuScene->StaticDrawRange().start,
        scene.gpuScene->StaticDrawRange().count,
        sizeof(DrawIndexedIndirectCommand),
        _context->VulkanContext()->Dldi());

    _context->GetDrawStats().IndirectDraw(scene.gpuScene->DrawCount(), scene.gpuScene->DrawCommandIndexCount());

    commandBuffer.endRenderingKHR(vkContext->Dldi());
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
        .SetDepthAttachmentFormat(_context->Resources()->ImageResourceManager().Access(_gBuffers.Shadow())->format)
        .BuildPipeline(_pipeline, _pipelineLayout);
}

void ShadowPipeline::CreateDrawBufferDescriptorSet(const GPUScene& gpuScene)
{
    auto vkContext { _context->VulkanContext() };

    vk::DescriptorSetLayout layout = gpuScene.DrawBufferLayout();
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = vkContext->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    util::VK_ASSERT(vkContext->Device().allocateDescriptorSets(&allocateInfo, &_drawBufferDescriptorSet),
        "Failed allocating descriptor sets!");

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer);

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

    vkContext->Device().updateDescriptorSets({ bufferWrite }, {});
}
