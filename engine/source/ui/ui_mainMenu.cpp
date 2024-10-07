//
// Created by luuk on 30-9-2024.
//
#include "ui/ui_mainMenu.hpp"
#include "pch.hpp"
#include "vulkan_helper.hpp"
#include "ui/ui_text.hpp"
void MainMenuCanvas::InitElements(const VulkanBrain& brain)
{
    // demo scene
    auto sampler = util::CreateSampler(brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eNearest, 0);

    ImageCreation buttonNormalImage;

    buttonNormalImage.format = vk::Format::eR8G8B8A8Unorm;
    buttonNormalImage.sampler = sampler.get();
    buttonNormalImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
    buttonNormalImage.LoadFromFile("assets/textures/buttonBackGround.png");
    buttonNormalImage.isHDR = false;
    auto normalImage = brain.GetImageResourceManager().Create(buttonNormalImage);

    buttonNormalImage.LoadFromFile("assets/textures/buttonHovered.png");
    auto hoveredImage = brain.GetImageResourceManager().Create(buttonNormalImage);

    brain.UpdateBindlessSet();
    std::unique_ptr<UIButton> playbutton = std::make_unique<UIButton>();
    playbutton->m_Style.m_NormalImage = normalImage;
    playbutton->m_Style.m_PressedImage = normalImage;
    playbutton->m_Style.m_HoveredImage = hoveredImage;
    playbutton->m_AnchorPoint = UIElement::AnchorPoint::TOP_LEFT;
    playbutton->m_Scale = { 300, 100 };
    playbutton->SetLocation({ 100, 100 });
    playbutton->m_OnBeginHoverCallBack = []() {};
    playbutton->m_OnMouseDownCallBack = []() {};

    // std::unique_ptr<UITextElement> text = std::make_unique<UITextElement>();
    // text->m_Text = "play";
    // playbutton->AddChild(std::move(text));
    // AddChild(std::move(playbutton));

    UpdateChildAbsoluteLocations();
}