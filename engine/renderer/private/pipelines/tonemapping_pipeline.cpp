#include "pipelines/tonemapping_pipeline.hpp"

#include "bloom_settings.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <graphics_resources.hpp>
#include <resource_management/image_resource_manager.hpp>

TonemappingPipeline::TonemappingPipeline(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<GPUImage> hdrTarget, ResourceHandle<GPUImage> bloomTarget, ResourceHandle<GPUImage> outputTarget, const SwapChain& _swapChain, const BloomSettings& bloomSettings)
    : _context(context)
    , _swapChain(_swapChain)
    , _hdrTarget(hdrTarget)
    , _bloomTarget(bloomTarget)
    , _outputTarget(outputTarget)
    , _bloomSettings(bloomSettings)
{
    CreatePipeline();

    _pushConstants.hdrTargetIndex = hdrTarget.Index();
    _pushConstants.bloomTargetIndex = bloomTarget.Index();
}

TonemappingPipeline::~TonemappingPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void TonemappingPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _context->Resources()->ImageResourceManager().Access(_outputTarget)->views[0],
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

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, _pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, { _bloomSettings.GetDescriptorSetData(currentFrame) }, {});

    // Fullscreen triangle.
    commandBuffer.draw(3, 1, 0, 0);

    _context->GetDrawStats().Draw(3);

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
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
        .SetColorAttachmentFormats({ _context->Resources()->ImageResourceManager().Access(_outputTarget)->format })
        .SetDepthAttachmentFormat(vk::Format::eUndefined)
        .BuildPipeline(_pipeline, _pipelineLayout);
}
