#include "pipelines/skydome_pipeline.hpp"

#include "../vulkan_helper.hpp"
#include "batch_buffer.hpp"
#include "bloom_settings.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

SkydomePipeline::SkydomePipeline(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUMesh> sphere, ResourceHandle<GPUImage> hdrTarget,
    ResourceHandle<GPUImage> brightnessTarget, ResourceHandle<GPUImage> environmentMap, const GBuffers& gBuffers, const BloomSettings& bloomSettings)
    : _context(context)
    , _hdrTarget(hdrTarget)
    , _brightnessTarget(brightnessTarget)
    , _environmentMap(environmentMap)
    , _gBuffers(gBuffers)
    , _sphere(sphere)
    , _bloomSettings(bloomSettings)
{
    CreatePipeline();

    _pushConstants.hdriIndex = environmentMap.Index();
}

SkydomePipeline::~SkydomePipeline()
{
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
    _context->VulkanContext()->Device().destroy(_pipeline);
}

void SkydomePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Skydome Pipeline");
    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eNone;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eNone;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    std::array<vk::RenderingAttachmentInfoKHR, 2> colorAttachmentInfos {};

    // HDR color
    colorAttachmentInfos[0].imageView = _context->Resources()->ImageResourceManager().Access(_hdrTarget)->views[0];
    colorAttachmentInfos[0].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[0].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[0].loadOp = vk::AttachmentLoadOp::eLoad;

    // HDR brightness for bloom
    colorAttachmentInfos[1].imageView = _context->Resources()->ImageResourceManager().Access(_brightnessTarget)->views[0];
    colorAttachmentInfos[1].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[1].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[1].loadOp = vk::AttachmentLoadOp::eLoad;

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { _context->Resources()->ImageResourceManager().Access(_hdrTarget)->width,
        _context->Resources()->ImageResourceManager().Access(_hdrTarget)->height };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(_pushConstants), &_pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->VertexBuffer())->buffer;
    vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->IndexBuffer())->buffer;

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.staticBatchBuffer->IndexType());

    auto sphere = _context->Resources()->MeshResourceManager().Access(_sphere);
    commandBuffer.drawIndexed(sphere->count, 1, sphere->indexOffset, sphere->vertexOffset, 0);

    _context->GetDrawStats().Draw(sphere->count);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
}

void SkydomePipeline::CreatePipeline()
{
    std::array<vk::PipelineColorBlendAttachmentState, 2> blendAttachments {};
    blendAttachments[0].blendEnable = vk::False;
    blendAttachments[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    blendAttachments[1] = blendAttachments[0];

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = blendAttachments.size();
    colorBlendStateCreateInfo.pAttachments = blendAttachments.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = vk::True;
    depthStencilStateCreateInfo.depthWriteEnable = vk::False;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eGreaterOrEqual;
    depthStencilStateCreateInfo.depthBoundsTestEnable = vk::True;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 0.0f;
    depthStencilStateCreateInfo.stencilTestEnable = vk::False;

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfoKHR> structureChain;

    std::vector<vk::Format> formats = {
        _context->Resources()->ImageResourceManager().Access(_hdrTarget)->format,
        _context->Resources()->ImageResourceManager().Access(_brightnessTarget)->format
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/skydome.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/skydome.frag.spv");

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
