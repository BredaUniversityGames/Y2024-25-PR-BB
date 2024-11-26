#include "pipelines/ui_pipeline.hpp"

#include "gpu_scene.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"

#include <graphics_context.hpp>
#include <pipeline_builder.hpp>
#include <vulkan_context.hpp>

#include "vulkan_helper.hpp"

UIPipeline::UIPipeline(const std::shared_ptr<GraphicsContext>& context, const SwapChain& swapChain)
    : _context(context)
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
    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
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
        .SetColorAttachmentFormats({ _swapChain.GetFormat() })
        .SetDepthAttachmentFormat(vk::Format::eUndefined)
        .BuildPipeline(_pipeline, _pipelineLayout);
}
void UIPipeline::RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR const finalColorAttachmentInfo {
        .imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex),
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfoKHR const renderingInfo {
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

    const glm::mat4 projectionMatrix = glm::ortho(0.f, static_cast<float>(_swapChain.GetExtent().width), static_cast<float>(_swapChain.GetExtent().height), 0.f, -1.f, 1.f);

    UIPushConstants pushConstants;
    for (auto& i : _drawlist)
    {
        pushConstants.quad = i;
        pushConstants.quad.projection = projectionMatrix * pushConstants.quad.projection;
        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(UIPushConstants), &pushConstants);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});

        commandBuffer.draw(6, 1, 0, 0);
        _context->GetDrawStats().Draw(6);
    }
    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
    _drawlist.clear();
}
