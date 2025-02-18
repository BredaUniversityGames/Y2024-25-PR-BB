#include "passes/shadow_pass.hpp"

#include "batch_buffer.hpp"
#include "camera_batch.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

#include <vector>

ShadowPass::ShadowPass(const std::shared_ptr<GraphicsContext>& context, const GPUScene& gpuScene, const CameraBatch& cameraBatch)
    : _context(context)
    , _cameraBatch(cameraBatch)
{
    CreateStaticPipeline(gpuScene);
    CreateSkinnedPipeline(gpuScene);
}

ShadowPass::~ShadowPass()
{
    _context->VulkanContext()->Device().destroy(_staticPipeline);
    _context->VulkanContext()->Device().destroy(_staticPipelineLayout);
    _context->VulkanContext()->Device().destroy(_skinnedPipeline);
    _context->VulkanContext()->Device().destroy(_skinnedPipelineLayout);
}

void ShadowPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Shadow Pass");

    static bool isPrepass = true;
    DrawGeometry(commandBuffer, currentFrame, scene, isPrepass);
    isPrepass = !isPrepass;
}

void ShadowPass::CreateStaticPipeline(const GPUScene& gpuScene)
{
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f,
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/shadow.vert.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(vk::PipelineColorBlendStateCreateInfo {})
                      .SetDepthStencilState(depthStencilStateCreateInfo)
                      .SetRasterizationState(rasterizationStateCreateInfo)
                      .SetColorAttachmentFormats({})
                      .SetDepthAttachmentFormat(_context->Resources()->ImageResourceManager().Access(gpuScene.StaticShadow())->format)
                      .BuildPipeline();

    _staticPipelineLayout = std::get<0>(result);
    _staticPipeline = std::get<1>(result);
}

void ShadowPass::CreateSkinnedPipeline(const GPUScene& gpuScene)
{
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eFront,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f,
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/skinned_shadow.vert.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(vk::PipelineColorBlendStateCreateInfo {})
                      .SetDepthStencilState(depthStencilStateCreateInfo)
                      .SetRasterizationState(rasterizationStateCreateInfo)
                      .SetColorAttachmentFormats({})
                      .SetDepthAttachmentFormat(_context->Resources()->ImageResourceManager().Access(gpuScene.StaticShadow())->format)
                      .BuildPipeline();

    _skinnedPipelineLayout = std::get<0>(result);
    _skinnedPipeline = std::get<1>(result);
}

void ShadowPass::DrawGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool prepass)
{
    auto vkContext { _context->VulkanContext() };
    auto resources { _context->Resources() };

    const auto* staticShadowImage = resources->ImageResourceManager().Access(scene.gpuScene->StaticShadow());
    const auto* dynamicShadowImage = resources->ImageResourceManager().Access(scene.gpuScene->DynamicShadow());

    if (scene.gpuScene->ShouldUpdateShadows() == true)
    {
        vk::RenderingAttachmentInfoKHR depthAttachmentInfo {
            .imageView = staticShadowImage->view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = prepass ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = { .depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 } },
        };

        vk::RenderingInfoKHR renderingInfo {
            .renderArea = {
                .extent = vk::Extent2D { staticShadowImage->width, staticShadowImage->height } },
            .layerCount = 1,
            .pDepthAttachment = &depthAttachmentInfo,
        };

        commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

        if (scene.gpuScene->StaticDrawCount() > 0)
        {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _staticPipeline);

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 0, { scene.gpuScene->GetStaticInstancesDescriptorSet(currentFrame) }, {});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 1, { scene.gpuScene->GetSceneDescriptorSet(currentFrame) }, {});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 2, { _cameraBatch.StaticDraw().redirectDescriptor }, {});

            vk::Buffer vertexBuffer = resources->BufferResourceManager().Access(scene.staticBatchBuffer->VertexBuffer())->buffer;
            vk::Buffer indexBuffer = resources->BufferResourceManager().Access(scene.staticBatchBuffer->IndexBuffer())->buffer;
            vk::Buffer indirectDrawBuffer = resources->BufferResourceManager().Access(_cameraBatch.StaticDraw().drawBuffer)->buffer;
            vk::Buffer countBuffer = resources->BufferResourceManager().Access(_cameraBatch.StaticDraw().redirectBuffer)->buffer;

            commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
            commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.staticBatchBuffer->IndexType());

            commandBuffer.drawIndexedIndirectCountKHR(
                indirectDrawBuffer,
                0,
                countBuffer,
                0,
                scene.gpuScene->StaticDrawCount(),
                sizeof(DrawIndexedIndirectCommand),
                _context->VulkanContext()->Dldi());

            _context->GetDrawStats().IndirectDraw(scene.gpuScene->StaticDrawCount(), scene.gpuScene->DrawCommandIndexCount(scene.gpuScene->StaticDrawCommands()));
        }
        commandBuffer.endRenderingKHR(vkContext->Dldi());
    }

    {
        vk::RenderingAttachmentInfoKHR depthAttachmentInfo {
            .imageView = dynamicShadowImage->view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = prepass ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = { .depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 } },
        };

        vk::RenderingInfoKHR renderingInfo {
            .renderArea = {
                .extent = vk::Extent2D { dynamicShadowImage->width, dynamicShadowImage->height } },
            .layerCount = 1,
            .pDepthAttachment = &depthAttachmentInfo,
        };

        commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

        if (scene.gpuScene->SkinnedDrawCount() > 0)
        {
            commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _skinnedPipeline);

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 0, { scene.gpuScene->GetSkinnedInstancesDescriptorSet(currentFrame) }, {});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 1, { scene.gpuScene->GetSceneDescriptorSet(currentFrame) }, {});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 2, { _cameraBatch.SkinnedDraw().redirectDescriptor }, {});
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 3, { scene.gpuScene->GetSkinDescriptorSet(currentFrame) }, {});

            vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->VertexBuffer())->buffer;
            vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->IndexBuffer())->buffer;
            vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.SkinnedDraw().drawBuffer)->buffer;
            vk::Buffer countBuffer = resources->BufferResourceManager().Access(_cameraBatch.SkinnedDraw().redirectBuffer)->buffer;

            commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
            commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.skinnedBatchBuffer->IndexType());

            commandBuffer.drawIndexedIndirectCountKHR(
                indirectDrawBuffer,
                0,
                countBuffer,
                0,
                scene.gpuScene->SkinnedDrawCount(),
                sizeof(DrawIndexedIndirectCommand),
                _context->VulkanContext()->Dldi());

            _context->GetDrawStats().IndirectDraw(scene.gpuScene->SkinnedDrawCount(), scene.gpuScene->DrawCommandIndexCount(scene.gpuScene->SkinnedDrawCommands()));
        }

        commandBuffer.endRenderingKHR(vkContext->Dldi());
    }
}
