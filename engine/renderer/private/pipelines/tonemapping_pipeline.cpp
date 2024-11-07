#include "pipelines/tonemapping_pipeline.hpp"
#include "bloom_settings.hpp"
#include "gpu_scene.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_helper.hpp"

TonemappingPipeline::TonemappingPipeline(const VulkanContext& brain, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> bloomTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings)
    : _brain(brain)
    , _swapChain(_swapChain)
    , _hdrTarget(hdrTarget)
    , _bloomTarget(bloomTarget)
    , _bloomSettings(bloomSettings)
{
    CreatePipeline();

    _pushConstants.hdrTargetIndex = hdrTarget.index;
    _pushConstants.bloomTargetIndex = bloomTarget.index;
}

TonemappingPipeline::~TonemappingPipeline()
{
    _brain.device.destroy(_pipeline);
    _brain.device.destroy(_pipelineLayout);
}

void TonemappingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex);
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    finalColorAttachmentInfo.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = _swapChain.GetExtent();
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(_pushConstants), &_pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_brain.bindlessSet, 0, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1, &_bloomSettings.GetDescriptorSetData(currentFrame), 0, nullptr);

    // Fullscreen triangle.
    commandBuffer.draw(3, 1, 0, 0);
    _brain.drawStats.indexCount += 3;
    _brain.drawStats.drawCalls++;

    commandBuffer.endRenderingKHR(_brain.dldi);
}

void TonemappingPipeline::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {};
    colorBlendAttachmentState.blendEnable = vk::False;
    colorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/tonemapping.frag.spv");

    PipelineBuilder pipelineBuilder { _brain };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats({ _swapChain.GetFormat() })
        .SetDepthAttachmentFormat(vk::Format::eUndefined)
        .BuildPipeline(_pipeline, _pipelineLayout);
}
