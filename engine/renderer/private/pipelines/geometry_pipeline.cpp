#include "pipelines/geometry_pipeline.hpp"

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

GeometryPipeline::GeometryPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, const CameraResource& camera, const GPUScene& gpuScene)
    : _context(context)
    , _gBuffers(gBuffers)
    , _camera(camera)
    , _culler(_context, gpuScene)
{
    CreatePipeline();

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
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void GeometryPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    _culler.RecordCommands(commandBuffer, currentFrame, scene, _camera, _drawBuffer, _drawBufferDescriptorSet);

    std::array<vk::RenderingAttachmentInfoKHR, DEFERRED_ATTACHMENT_COUNT> colorAttachmentInfos {};
    for (size_t i = 0; i < colorAttachmentInfos.size(); ++i)
    {
        vk::RenderingAttachmentInfoKHR& info { colorAttachmentInfos[i] };
        info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        info.storeOp = vk::AttachmentStoreOp::eStore;
        info.loadOp = vk::AttachmentLoadOp::eClear;
        info.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    }

    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        colorAttachmentInfos[i].imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments()[i])->view;

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    glm::uvec2 displaySize = _gBuffers.Size();
    renderingInfo.renderArea.extent = vk::Extent2D { displaySize.x, displaySize.y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { _camera.DescriptorSet(currentFrame) }, {});

    vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.batchBuffer->VertexBuffer())->buffer;
    vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.batchBuffer->IndexBuffer())->buffer;
    vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer)->buffer;
    vk::Buffer indirectCountBuffer = _context->Resources()->BufferResourceManager().Access(scene.gpuScene->IndirectCountBuffer(currentFrame))->buffer;
    uint32_t indirectCountOffset = scene.gpuScene->IndirectCountOffset();

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.batchBuffer->IndexType());
    commandBuffer.drawIndexedIndirectCountKHR(indirectDrawBuffer, 0, indirectCountBuffer, indirectCountOffset, scene.gpuScene->DrawCount(), sizeof(vk::DrawIndexedIndirectCommand), _context->VulkanContext()->Dldi());

    _context->GetDrawStats().IndirectDraw(scene.gpuScene->DrawCount(), scene.gpuScene->DrawCommandIndexCount());

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

void GeometryPipeline::CreatePipeline()
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
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    std::vector<vk::Format> formats(DEFERRED_ATTACHMENT_COUNT);
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        formats[i] = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments()[i])->format;

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/geom.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/geom.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetDepthStencilState(depthStencilStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
        .BuildPipeline(_pipeline, _pipelineLayout);
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
