#include "pipelines/geometry_pipeline.hpp"

#include "batch_buffer.hpp"
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
#include "vertex.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <entt/entt.hpp>

GeometryPipeline::GeometryPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const GPUScene& gpuScene)
    : _context(context)
    , _gBuffers(gBuffers)
    , _culler(_context, gpuScene)
{
    CreateStaticPipeline();
    CreateSkinnedPipeline();

    auto mainDrawBufferHandle = gpuScene.IndirectDrawBuffer(0);
    const auto* mainDrawBuffer = _context->Resources()->BufferResourceManager().Access(mainDrawBufferHandle);

    BufferCreation creation {
        .size = mainDrawBuffer->size,
        .usage = mainDrawBuffer->usage,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = "Geometry draw buffer",
    };

    _drawBuffer = _context->Resources()->BufferResourceManager().Create(creation);

    CreateDrawBufferDescriptorSet(gpuScene);
}

GeometryPipeline::~GeometryPipeline()
{
    _context->VulkanContext()->Device().destroy(_staticPipeline);
    _context->VulkanContext()->Device().destroy(_staticPipelineLayout);
    _context->VulkanContext()->Device().destroy(_skinnedPipeline);
    _context->VulkanContext()->Device().destroy(_skinnedPipelineLayout);
}

void GeometryPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    _culler.RecordCommands(commandBuffer, currentFrame, scene, scene.gpuScene->MainCamera(), _drawBuffer, _drawBufferDescriptorSet);

    std::array<vk::RenderingAttachmentInfoKHR, DEFERRED_ATTACHMENT_COUNT> colorAttachmentInfos {};
    for (size_t i = 0; i < colorAttachmentInfos.size(); ++i)
    {
        vk::RenderingAttachmentInfoKHR& info { colorAttachmentInfos.at(i) };
        info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        info.storeOp = vk::AttachmentStoreOp::eStore;
        info.loadOp = vk::AttachmentLoadOp::eClear;
        info.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    }

    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
    {
        colorAttachmentInfos.at(i).imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments().at(i))->view;
    }

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
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
    if (scene.gpuScene->StaticDrawRange().count > 0)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _staticPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 1, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 2, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});

        vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->VertexBuffer())->buffer;
        vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->IndexBuffer())->buffer;
        vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer)->buffer;

        commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
        commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.staticBatchBuffer->IndexType());
        commandBuffer.drawIndexedIndirect(
            indirectDrawBuffer,
            scene.gpuScene->StaticDrawRange().start * sizeof(DrawIndexedIndirectCommand),
            scene.gpuScene->StaticDrawRange().count,
            sizeof(DrawIndexedIndirectCommand),
            _context->VulkanContext()->Dldi());

        _context->GetDrawStats().IndirectDraw(scene.gpuScene->StaticDrawRange().count, scene.gpuScene->DrawCommandIndexCount(scene.gpuScene->StaticDrawRange()));
    }

    if (scene.gpuScene->SkinnedDrawRange().count > 0)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _skinnedPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 1, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 2, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 3, { scene.gpuScene->GetSkinDescriptorSet(currentFrame) }, {});

        vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->VertexBuffer())->buffer;
        vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->IndexBuffer())->buffer;
        vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer)->buffer;

        commandBuffer.pushConstants<uint32_t>(_skinnedPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, { scene.gpuScene->SkinnedDrawRange().start });

        commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
        commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.skinnedBatchBuffer->IndexType());
        commandBuffer.drawIndexedIndirect(
            indirectDrawBuffer,
            scene.gpuScene->SkinnedDrawRange().start * sizeof(DrawIndexedIndirectCommand),
            scene.gpuScene->SkinnedDrawRange().count,
            sizeof(DrawIndexedIndirectCommand),
            _context->VulkanContext()->Dldi());

        _context->GetDrawStats().IndirectDraw(scene.gpuScene->SkinnedDrawRange().count, scene.gpuScene->DrawCommandIndexCount(scene.gpuScene->SkinnedDrawRange()));
    }

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

void GeometryPipeline::CreateStaticPipeline()
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

void GeometryPipeline::CreateSkinnedPipeline()
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

    std::vector<vk::Format> formats(DEFERRED_ATTACHMENT_COUNT);
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        formats.at(i) = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments().at(i))->format;

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

void GeometryPipeline::CreateDrawBufferDescriptorSet(const GPUScene& gpuScene)
{
    vk::DescriptorSetLayout layout = gpuScene.DrawBufferLayout();
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, &_drawBufferDescriptorSet),
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

    _context->VulkanContext()->Device().updateDescriptorSets(1, &bufferWrite, 0, nullptr);
}
