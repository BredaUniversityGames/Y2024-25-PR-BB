#include "passes/geometry_pass.hpp"

#include "batch_buffer.hpp"
#include "camera_batch.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "constants.hpp"
#include "ecs_module.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

GeometryPass::GeometryPass(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const CameraBatch& cameraBatch)
    : _context(context)
    , _gBuffers(gBuffers)
    , _cameraBatch(cameraBatch)
{
    CreateStaticPipeline();
    CreateSkinnedPipeline();
}

GeometryPass::~GeometryPass()
{
    _context->VulkanContext()->Device().destroy(_staticPipeline);
    _context->VulkanContext()->Device().destroy(_staticPipelineLayout);
    _context->VulkanContext()->Device().destroy(_skinnedPipeline);
    _context->VulkanContext()->Device().destroy(_skinnedPipelineLayout);
}

void GeometryPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Geometry Pass");

    static bool isPrepass = true;
    DrawGeometry(commandBuffer, currentFrame, scene, isPrepass);
    isPrepass = !isPrepass;
}

void GeometryPass::CreateStaticPipeline()
{
    std::array<vk::PipelineColorBlendAttachmentState, DEFERRED_ATTACHMENT_COUNT> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eGreater;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    std::vector<vk::Format> formats(DEFERRED_ATTACHMENT_COUNT);
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        formats.at(i) = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments().at(i))->format;

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/geom.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/geom.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetDepthStencilState(depthStencilStateCreateInfo)
                      .SetColorAttachmentFormats(formats)
                      .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
                      .BuildPipeline();

    _staticPipelineLayout = std::get<0>(result);
    _staticPipeline = std::get<1>(result);
}

void GeometryPass::CreateSkinnedPipeline()
{
    std::array<vk::PipelineColorBlendAttachmentState, DEFERRED_ATTACHMENT_COUNT> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eGreaterOrEqual;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    const ImageResourceManager& imageResourceManager = _context->Resources()->ImageResourceManager();

    std::vector<vk::Format> formats(DEFERRED_ATTACHMENT_COUNT);
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
    {
        formats.at(i) = imageResourceManager.Access(_gBuffers.Attachments().at(i))->format;
    }

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/skinned_geom.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/geom.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetDepthStencilState(depthStencilStateCreateInfo)
                      .SetColorAttachmentFormats(formats)
                      .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
                      .BuildPipeline();

    _skinnedPipelineLayout = std::get<0>(result);
    _skinnedPipeline = std::get<1>(result);
}

void GeometryPass::DrawGeometry(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene, bool prepass)
{
    std::array<vk::RenderingAttachmentInfoKHR, DEFERRED_ATTACHMENT_COUNT> colorAttachmentInfos {};
    for (size_t i = 0; i < colorAttachmentInfos.size(); ++i)
    {
        vk::RenderingAttachmentInfoKHR& info { colorAttachmentInfos.at(i) };
        info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        info.storeOp = vk::AttachmentStoreOp::eStore;
        info.loadOp = prepass ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
        info.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    }

    const ImageResourceManager& imageResourceManager = _context->Resources()->ImageResourceManager();
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
    {
        colorAttachmentInfos.at(i).imageView = imageResourceManager.Access(_gBuffers.Attachments().at(i))->view;
    }

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = imageResourceManager.Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.loadOp = prepass ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 0.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    glm::uvec2 displaySize = _gBuffers.Size();
    renderingInfo.renderArea.extent = vk::Extent2D { displaySize.x, displaySize.y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());
    if (scene.gpuScene->StaticDrawCount() > 0)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _staticPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 1, { scene.gpuScene->GetStaticInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 2, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 3, { scene.gpuScene->MainCameraBatch().StaticDraw().redirectDescriptor }, {});

        vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->VertexBuffer())->buffer;
        vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->IndexBuffer())->buffer;
        vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.StaticDraw().drawBuffer)->buffer;
        vk::Buffer countBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.StaticDraw().redirectBuffer)->buffer;

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

    if (scene.gpuScene->SkinnedDrawCount() > 0)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _skinnedPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 1, { scene.gpuScene->GetSkinnedInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 2, { _cameraBatch.Camera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 3, { _cameraBatch.SkinnedDraw().redirectDescriptor }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 4, { scene.gpuScene->GetSkinDescriptorSet(currentFrame) }, {});

        vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->VertexBuffer())->buffer;
        vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->IndexBuffer())->buffer;
        vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.SkinnedDraw().drawBuffer)->buffer;
        vk::Buffer countBuffer = _context->Resources()->BufferResourceManager().Access(_cameraBatch.SkinnedDraw().redirectBuffer)->buffer;

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

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}
