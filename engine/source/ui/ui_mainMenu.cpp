//
// Created by luuk on 30-9-2024.
//
#include "ui/ui_mainMenu.hpp"
#include "pch.hpp"
#include "vulkan_helper.hpp"
#include "ui/ui_text_rendering.hpp"
void MainMenuCanvas::InitElements(const VulkanBrain& brain)
{

    auto sampler = util::CreateSampler(brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eNearest, 0);

    ImageCreation buttonNormalImage;

    buttonNormalImage.format = vk::Format::eR8G8B8A8Unorm;
    buttonNormalImage.sampler = sampler.get();
    buttonNormalImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
    buttonNormalImage.LoadFromFile("assets/textures/buttonBackGround.png");
    buttonNormalImage.isHDR = false;
    auto normalImage = brain.ImageResourceManager().Create(buttonNormalImage);

    buttonNormalImage.LoadFromFile("assets/textures/buttonBackGroundHovered.png");
    auto hoveredImage = brain.ImageResourceManager().Create(buttonNormalImage);

    brain.UpdateBindlessSet();
    std::unique_ptr<UIButton> playbutton = std::make_unique<UIButton>();
    playbutton->m_NormalImage = normalImage;
    playbutton->m_PressedImage = normalImage;
    playbutton->m_HoveredImage = hoveredImage;
    playbutton->m_AnchorPoint = UIElement::AnchorPoint::TOP_LEFT;
    playbutton->Scale = { 300, 100 };
    playbutton->SetLocation({ 100, 100 });
    playbutton->m_OnBeginHoverCallBack = []() {};
    playbutton->m_OnMouseDownCallBack = []() {};

    std::unique_ptr<UITextElement> text = std::make_unique<UITextElement>();
    text->m_Text = "play";
    playbutton->AddChild(std::move(text));
    AddChild(std::move(playbutton));

    UpdateChildAbsoluteLocations();
}