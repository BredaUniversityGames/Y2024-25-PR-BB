#include "game_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"
#include "ui_toggle.hpp"

std::shared_ptr<SettingsMenu> SettingsMenu::Create(
    GameModule& gameModule,
    GraphicsContext& graphicsContext,
    const glm::uvec2& screenResolution,
    std::shared_ptr<UIFont> font)
{
    auto settings = std::make_shared<SettingsMenu>(screenResolution);

    settings->anchorPoint = UIElement::AnchorPoint::eMiddle;
    settings->SetAbsoluteTransform(settings->GetAbsoluteLocation(), screenResolution);

    SamplerCreation samplerCreation;
    samplerCreation.minFilter = vk::Filter::eNearest;
    samplerCreation.magFilter = vk::Filter::eNearest;
    static ResourceHandle<Sampler> sampler = graphicsContext.Resources()->SamplerResourceManager().Create(samplerCreation);

    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;
        commonImageData.name = "BlackBackdrop";

        constexpr std::byte black = {};
        constexpr std::byte transparent = static_cast<std::byte>(150);
        commonImageData.initialData = { black, black, black, transparent };

        auto backdropImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData);
        auto image = settings->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
        image->anchorPoint = UIElement::AnchorPoint::eFill;
    }

    // Button Style
    UIButton::ButtonStyle buttonStyle {};
    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        buttonStyle.normalImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button.png"), sampler);
        buttonStyle.hoveredImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_2.png"), sampler);
        buttonStyle.pressedImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_selected.png"), sampler);
    }

    UIToggle::ToggleStyle toggleStyle {};

    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        toggleStyle.empty = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/circleempty.png"), sampler);
        toggleStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/circlefull.png"), sampler);
    }

    // auto buttonPanel = settings->AddChild<Canvas>(glm::vec2 { 0.0f, 0.0f });

    // {
    //     buttonPanel->anchorPoint = UIElement::AnchorPoint::eMiddle;
    //     // buttonPanel->SetLocation(glm::vec2(screenResFloat.y * 0.1f, screenResFloat.y * 0.4f));
    // }

    {
        // SETTINGS
        {
            settings->AddChild<UITextElement>(font, "Settings", glm::vec2 { 0.0f, -550.0f }, 100);
        }

        constexpr float textSize = 60;

        glm::vec2 elemPos = { 0.0f, -400.0f };
        constexpr glm::vec2 rowSize = { 1000.0f, 0.0f };
        constexpr glm::vec2 increment = { 0.0f, 100.0f };

        constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 5.0f;
        constexpr glm::vec2 toggleSize = glm::vec2(22, 22) * 3.0f;

        constexpr glm::vec2 toggleOffset = glm::vec2(400.0f, -toggleSize.y * 0.25f + textSize * 0.25f);

        // FRAME COUNTER
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Toggle Framerate Display", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto toggle = node->AddChild<UIToggle>(toggleStyle, toggleOffset, toggleSize);
            toggle->anchorPoint = UIElement::AnchorPoint::eTopRight;

            toggle->state = gameModule.GetSettings().framerateCounter;

            auto callback = [&gameModule](bool val)
            { gameModule.GetSettings().framerateCounter = val; };

            toggle->OnToggle(callback);
        }

        // Sensitivity
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Aim Sensitivity", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        }

        // Gamma
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Screen brightness", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        }

        // VSync
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Toggle VSync", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto toggle = node->AddChild<UIToggle>(toggleStyle, toggleOffset, toggleSize);
            toggle->anchorPoint = UIElement::AnchorPoint::eTopRight;
        }

        // UI Scale
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Set UI Scale", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        }

        // Master Volume
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Master Volume", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        }

        // Music Volume
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Music Volume", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        }

        // SFX Volume
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "SFX Volume", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        }

        // BACK BUTTON
        {
            constexpr glm::vec2 backPos = { 0.0f, 500.0f };
            auto back = settings->AddChild<UIButton>(buttonStyle, backPos, buttonBaseSize);
            back->anchorPoint = UIElement::AnchorPoint::eMiddle;
            back->AddChild<UITextElement>(font, "Back", textSize)->SetColor(glm::vec4(1, 1, 1, 1));

            back->OnPress(Callback { [&gameModule]()
                { gameModule.PopUIMenu(); } });
        }
    }

    settings->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return settings;
}
