#include "passes/ui_pass.hpp"

#include "fastgltf/types.hpp"
#include "glm/gtx/string_cast.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "swap_chain.hpp"
#include "vulkan_helper.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
UIPass::UIPass(const std::shared_ptr<GraphicsContext>& context, const ResourceHandle<GPUImage>& outputTarget, const SwapChain& swapChain)
    : _context(context)
    , _outputTarget(outputTarget)
    , _swapChain(swapChain)
{
    CreatePipeLine();

    SetProjectionMatrix(glm::vec2(_swapChain.GetExtent().width, _swapChain.GetExtent().height), glm::vec2(0));
}

UIPass::~UIPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void UIPass::CreatePipeLine()
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

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/ui.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/ui.frag.spv");
    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetColorAttachmentFormats({ _context->Resources()->ImageResourceManager().Access(_outputTarget)->format })
                      .SetDepthAttachmentFormat(vk::Format::eUndefined)
                      .BuildPipeline();

    _pipelineLayout = std::get<0>(result);
    _pipeline = std::get<1>(result);
}

void UIPass::RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "UI Pipeline");

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {
        .imageView = _context->Resources()->ImageResourceManager().Access(_outputTarget)->view,
        .imageLayout = vk::ImageLayout::eAttachmentOptimalKHR,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfoKHR renderingInfo {
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
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, { _context->BindlessSet() }, {});

    for (const auto& quad : _drawList)
    {
        _pushConstants.quad = quad;
        _pushConstants.quad.matrix = _projectionMatrix * _pushConstants.quad.matrix;

        commandBuffer.pushConstants<UIPushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, _pushConstants);

        commandBuffer.draw(6, 1, 0, 0);
        _context->GetDrawStats().Draw(6);
    }
    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());
    _drawList.clear();
}
void UIPass::SetProjectionMatrix(const glm::vec2& size, const glm::vec2& offset)
{
    _projectionMatrix = glm::ortho<float>(offset.x, offset.x + size.x, offset.y, offset.y + size.y);
}
