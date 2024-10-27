#include "ui_mainMenu.hpp"
#include "ui_core.hpp"
#include "../../include/pch.hpp"
#include "../../include/vulkan_helper.hpp"

MainMenuCanvas::MainMenuCanvas(const VulkanBrain& brain)
{
    // demo scene
    auto sampler = util::CreateSampler(brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eNearest, 0);

    ImageCreation buttonNormalImage;

    buttonNormalImage.format
        = vk::Format::eR8G8B8A8Unorm;
    buttonNormalImage.sampler = sampler.get();
    buttonNormalImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
    buttonNormalImage.LoadFromFile("assets/textures/buttonBackGround.png");
    buttonNormalImage.isHDR = false;

    auto normalImage = brain.GetImageResourceManager().Create(buttonNormalImage);

    buttonNormalImage.LoadFromFile("assets/textures/buttonHovered.png");
    auto hoveredImage = brain.GetImageResourceManager().Create(buttonNormalImage);

    UIButton::ButtonStyle standardStyle {
        .normalImage = normalImage,
        .hoveredImage = hoveredImage,
        .pressedImage = normalImage
    };

    brain.UpdateBindlessSet();
    std::unique_ptr<UIButton> playbutton = std::make_unique<UIButton>();
    playbutton->style = standardStyle;
    playbutton->anchorPoint = UIElement::AnchorPoint::eMiddle;
    playbutton->scale = { 300, 100 };
    playbutton->SetLocation({ 100, 100 });
    playbutton->onBeginHoverCallBack = []() {};
    playbutton->onMouseDownCallBack = []() {};

    // std::unique_ptr<UITextElement> text = std::make_unique<UITextElement>();
    // text->m_Text = "play";
    // playbutton->AddChild(std::move(text));
    AddChild(std::move(playbutton));

    UpdateChildAbsoluteLocations();
}