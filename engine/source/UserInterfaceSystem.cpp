//
// Created by luuk on 16-9-2024.
//

#include "ui/UserInterfaceSystem.hpp"

#include "swap_chain.hpp"
#include "vulkan_helper.hpp"
#include "pipelines/ui_pipelines.hpp"
#include "shaders/shader_loader.hpp"
#include "ui/fonts.hpp"
#include "ui/ui_button.hpp"
#include "ui/ui_text_rendering.hpp"

void UpdateUI(const InputManager& input, UIElement* element)
{
    element->Update(input);
}
void RenderUI(UIElement* element, UserInterfaceRenderer& context, const vk::CommandBuffer& cb, const VulkanBrain& b, SwapChain& swapChain, int swapChainIndex,
    const glm::mat4& projectionMatrix)
{

    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo {};
    finalColorAttachmentInfo.imageView = swapChain.GetImageView(swapChainIndex);
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eLoad;

    vk::RenderingInfoKHR renderingInfo {};
    renderingInfo.renderArea.extent = swapChain.GetExtent();
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    cb.beginRenderingKHR(&renderingInfo, b.dldi);

    element->SubmitDrawInfo(context);

    for (auto& i : context.m_UIRenderSystems)
    {
        i.second->Render(cb, projectionMatrix, b);
    }

    cb.endRenderingKHR(b.dldi);
    util::EndLabel(cb, b.dldi);
}

void Canvas::UpdateChildAbsoluteLocations()
{
    {
        for (const auto& i : GetChildren())
        {
            auto relativeLocation = i->GetRelativeLocation();
            switch (i->m_AnchorPoint)
            {
            case AnchorPoint::TOP_LEFT:
                i->UpdateAbsoluteLocation(AbsoluteLocation + relativeLocation);
                break;
            case AnchorPoint::TOP_RIGHT:
                i->UpdateAbsoluteLocation({ AbsoluteLocation.x + Scale.x - relativeLocation.x, AbsoluteLocation.y + relativeLocation.y });
                break;
            case AnchorPoint::BOTTOM_LEFT:
                i->UpdateAbsoluteLocation({ AbsoluteLocation.x + relativeLocation.x, AbsoluteLocation.y + Scale.y - relativeLocation.y });
                break;
            case AnchorPoint::BOTTOM_RIGHT:
                i->UpdateAbsoluteLocation(AbsoluteLocation + Scale - relativeLocation);
                break;
            }

            i->UpdateChildAbsoluteLocations();
        }
    }
}
void Canvas::SubmitDrawInfo(UserInterfaceRenderer& user_interface_render_context) const
{
    for (const auto& i : GetChildren())
    {
        i->SubmitDrawInfo(user_interface_render_context);
    }
}

void UserInterfaceRenderer::InitializeDefaultRenderSystems()
{
    std::shared_ptr<UIPipeLine> textPipeline = std::make_shared<UIPipeLine>(m_VulkanBrain);
    textPipeline->CreatePipeLine("shaders/ui_rendering-v.spv", "shaders/text_rendering-f.spv");

    std::shared_ptr<UIPipeLine> buttonPipeline = std::make_shared<UIPipeLine>(m_VulkanBrain);
    buttonPipeline->CreatePipeLine("shaders/ui_rendering-v.spv", "shaders/button_rendering-f.spv");

    AddRenderingSystem<UIButtonRenderSystem>(buttonPipeline);
    AddRenderingSystem<UITextRenderSystem>(textPipeline);
}
