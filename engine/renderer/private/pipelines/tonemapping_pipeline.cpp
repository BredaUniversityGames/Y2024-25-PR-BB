#include "pipelines/tonemapping_pipeline.hpp"

#include "bloom_settings.hpp"
#include "gpu_scene.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

TonemappingPipeline::TonemappingPipeline(const std::shared_ptr<VulkanContext>& context, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> bloomTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings)
    : _context(context)
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
    _context->Device().destroy(_pipeline);
    _context->Device().destroy(_pipelineLayout);
}

void TonemappingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex),
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } } },
    };

    vk::RenderingInfoKHR renderingInfo {
        .renderArea = {
            .offset = vk::Offset2D { 0, 0 },
            .extent = _swapChain.GetExtent(),
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &finalColorAttachmentInfo,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, _pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    // Fullscreen triangle.
    commandBuffer.draw(3, 1, 0, 0);

    // TODO: Fix this.
    //_context->drawStats.indexCount += 3;
    //_context->drawStats.drawCalls++;

    commandBuffer.endRenderingKHR(_context->Dldi());
}

void TonemappingPipeline::CreatePipeline()
{
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/fullscreen.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/tonemapping.frag.spv");

    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats({ _swapChain.GetFormat() })
        .SetDepthAttachmentFormat(vk::Format::eUndefined)
        .BuildPipeline(_pipeline, _pipelineLayout);
}
