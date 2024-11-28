#include "pipelines/ui_pipeline.hpp"

#include "gpu_scene.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"

#include "graphics_context.hpp"
#include "pipeline_builder.hpp"

#include "vulkan_helper.hpp"

#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"

UIPipeline::UIPipeline(const std::shared_ptr<GraphicsContext>& context, const ResourceHandle<GPUImage>& toneMappingTarget, const ResourceHandle<GPUImage>& uiTarget, const SwapChain& swapChain)
    : _context(context)
    , _toneMappingTarget(toneMappingTarget)
    , _uiTarget(uiTarget)
    , _swapChain(swapChain)
{
    CreatePipeLine();
}

UIPipeline::~UIPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void UIPipeline::CreatePipeLine()
{
    vk::PipelineColorBlendAttachmentState const colorBlendAttachmentState {
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo const colorBlendStateCreateInfo {
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
    };

    auto vertSpv = shader::ReadFile("shaders/bin/ui.vert.spv");
    auto fragSpv = shader::ReadFile("shaders/bin/ui.frag.spv");
    PipelineBuilder pipelineBuilder { _context };
    pipelineBuilder
        .AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv)
        .AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv)
        .SetColorBlendState(colorBlendStateCreateInfo)
        .SetColorAttachmentFormats({ _context->Resources()->ImageResourceManager().Access(_toneMappingTarget)->format })
        .SetDepthAttachmentFormat(vk::Format::eUndefined)
        .BuildPipeline(_pipeline, _pipelineLayout);
}
void UIPipeline::RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    const auto* toneMapping = _context->Resources()->ImageResourceManager().Access(_toneMappingTarget);
    const auto* ui = _context->Resources()->ImageResourceManager().Access(_uiTarget);

    util::TransitionImageLayout(commandBuffer, toneMapping->image, toneMapping->format, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal, 1, 0, 1);
    util::TransitionImageLayout(commandBuffer, ui->image, ui->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, 1, 0, 1);

    util::CopyImageToImage(commandBuffer, toneMapping->image, ui->image,
        vk::Extent2D { .width = toneMapping->width, .height = toneMapping->height }, vk::Extent2D { .width = ui->width, .height = ui->height });

    util::TransitionImageLayout(commandBuffer, toneMapping->image, toneMapping->format, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, 1);
    util::TransitionImageLayout(commandBuffer, ui->image, ui->format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eColorAttachmentOptimal, 1, 0, 1);

    vk::RenderingAttachmentInfoKHR const finalColorAttachmentInfo {
        .imageView = _context->Resources()->ImageResourceManager().Access(_uiTarget)->views[0],
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfoKHR const renderingInfo {
        .renderArea = {
            .offset = vk::Offset2D { .x = 0, .y = 0 },
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

    const glm::mat4 projectionMatrix = glm::ortho(0.f, static_cast<float>(_swapChain.GetExtent().width), 0.f, static_cast<float>(_swapChain.GetExtent().height));

    for (auto& i : _drawList)
    {
        _pushConstants.quad = i;
        _pushConstants.quad.projection = projectionMatrix * _pushConstants.quad.projection;
        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(UIPushConstants), &_pushConstants);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});

        commandBuffer.draw(6, 1, 0, 0);
        _context->GetDrawStats().Draw(6);
    }
    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
    _drawList.clear();
}
