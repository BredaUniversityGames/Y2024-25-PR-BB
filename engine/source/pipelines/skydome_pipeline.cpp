#include "pipelines/skydome_pipeline.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_helper.hpp"
#include "bloom_settings.hpp"
#include "batch_buffer.hpp"
#include "gpu_scene.hpp"
#include "pipeline_builder.hpp"

SkydomePipeline::SkydomePipeline(const VulkanBrain& brain, ResourceHandle<Mesh> sphere, const CameraResource& camera,
    ResourceHandle<Image> hdrTarget, ResourceHandle<Image> brightnessTarget, ResourceHandle<Image> environmentMap, const GBuffers& gBuffers, const BloomSettings& bloomSettings)
    : _brain(brain)
    , _camera(camera)
    , _hdrTarget(hdrTarget)
    , _brightnessTarget(brightnessTarget)
    , _environmentMap(environmentMap)
    , _gBuffers(gBuffers)
    , _sphere(sphere)
    , _bloomSettings(bloomSettings)
{
    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat,
        vk::SamplerMipmapMode::eLinear, 0);

    CreatePipeline();

    _pushConstants.hdriIndex = environmentMap.index;
}

SkydomePipeline::~SkydomePipeline()
{
    _brain.device.destroy(_pipelineLayout);
    _brain.device.destroy(_pipeline);
}

void SkydomePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = _brain.GetImageResourceManager().Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    vk::RenderingAttachmentInfoKHR stencilAttachmentInfo { depthAttachmentInfo };
    stencilAttachmentInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    stencilAttachmentInfo.loadOp = vk::AttachmentLoadOp::eDontCare;
    stencilAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    std::array<vk::RenderingAttachmentInfoKHR, 2> colorAttachmentInfos {};

    // HDR color
    colorAttachmentInfos[0].imageView = _brain.GetImageResourceManager().Access(_hdrTarget)->views[0];
    colorAttachmentInfos[0].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[0].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[0].loadOp = vk::AttachmentLoadOp::eLoad;

    // HDR brightness for bloom
    colorAttachmentInfos[1].imageView = _brain.GetImageResourceManager().Access(_brightnessTarget)->views[0];
    colorAttachmentInfos[1].imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    colorAttachmentInfos[1].storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentInfos[1].loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachmentInfos[1].clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = vk::Extent2D { _brain.GetImageResourceManager().Access(_hdrTarget)->width,
        _brain.GetImageResourceManager().Access(_hdrTarget)->height };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = util::HasStencilComponent(_gBuffers.DepthFormat()) ? &stencilAttachmentInfo : nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(_pushConstants), &_pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _brain.bindlessSet }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _camera.DescriptorSet(currentFrame) }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 2, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    vk::Buffer vertexBuffer = _brain.GetBufferResourceManager().Access(scene.batchBuffer.VertexBuffer())->buffer;
    vk::Buffer indexBuffer = _brain.GetBufferResourceManager().Access(scene.batchBuffer.IndexBuffer())->buffer;

    commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
    commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.batchBuffer.IndexType());

    auto sphere = _brain.GetMeshResourceManager().Access(_sphere);
    auto primitive = sphere->primitives[0];
    commandBuffer.drawIndexed(primitive.count, 1, primitive.indexOffset, primitive.vertexOffset, 0);
    _brain.drawStats.indexCount += primitive.count;
    _brain.drawStats.drawCalls++;

    commandBuffer.endRenderingKHR(_brain.dldi);
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
    depthStencilStateCreateInfo.depthWriteEnable = vk::True;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eLess;
    depthStencilStateCreateInfo.depthBoundsTestEnable = vk::False;

    std::vector<vk::Format> formats = {
        _brain.GetImageResourceManager().Access(_hdrTarget)->format,
        _brain.GetImageResourceManager().Access(_brightnessTarget)->format
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/skydome.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/skydome.frag.spv");

    PipelineBuilder pipelineBuilder { _brain };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetDepthStencilState(depthStencilStateCreateInfo)
        .SetColorAttachmentFormats(formats)
        .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
        .BuildPipeline(_pipeline, _pipelineLayout);
}
