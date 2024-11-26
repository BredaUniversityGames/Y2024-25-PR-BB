#include "pipelines/ui_pipeline.hpp"

#include "gpu_scene.hpp"
#include <graphics_context.hpp>
#include <pipeline_builder.hpp>

#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

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

void UIPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _swapChain.GetImageView(scene.targetSwapChainImageIndex),
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
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

    UIPushConstants pushConstants;
    for (auto& i : _drawlist)
    {

        pushConstants.quad = i;
        pushConstants.quad.projection = glm::ortho(static_cast<uint32_t>(0), _swapChain.GetExtent().width, _swapChain.GetExtent().height, static_cast<uint32_t>(0));
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});

        commandBuffer.pushConstants(_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(UIPushConstants), &pushConstants);
        commandBuffer.draw(6, 1, 0, 0);
        _context->GetDrawStats().Draw(6);
    }

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());

    _drawlist.clear();
}

void UIPipeline::CreatePipeLine()
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

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/ui.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/ui.frag.spv");

    PipelineBuilder reflector { _context };
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    reflector.SetColorBlendState(colorBlendStateCreateInfo);
    reflector.SetColorAttachmentFormats({ _swapChain.GetFormat() });
    reflector.SetDepthAttachmentFormat(vk::Format::eUndefined);
    reflector.BuildPipeline(_pipeline, _pipelineLayout);
}
