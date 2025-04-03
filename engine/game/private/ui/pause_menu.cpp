#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui/ui_menus.hpp"
#include "ui_button.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include <glm/glm.hpp>

std::shared_ptr<PauseMenu> PauseMenu::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution)
{
    auto pause = std::make_shared<PauseMenu>(screenResolution);

    auto font = LoadFromFile("assets/fonts/Rooters.ttf", 50, graphicsContext);
    pause->anchorPoint = UIElement::AnchorPoint::eMiddle;
    pause->SetAbsoluteTransform(pause->GetAbsoluteLocation(), screenResolution);

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
        auto image = pause->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
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

        buttonStyle.normalImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_st.png"));
        buttonStyle.hoveredImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_h.png"));
        buttonStyle.pressedImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_p.png"));
    }

    auto buttonPanel = pause->AddChild<Canvas>(glm::vec2 { 0.0f, 0.0f });

    {
        buttonPanel->anchorPoint = UIElement::AnchorPoint::eMiddle;
        // buttonPanel->SetLocation(glm::vec2(screenResFloat.y * 0.1f, screenResFloat.y * 0.4f));
    }

    {
        glm::vec2 buttonPos = { 0.0f, -300.0f };
        constexpr glm::vec2 increment = { 0.0f, 90.0f };
        constexpr glm::vec2 buttonBaseSize = glm::vec2(878, 243) * 0.5f;
        constexpr float textSize = 50;

        buttonPanel->AddChild<UITextElement>(font, "Paused", buttonPos, 200);
        buttonPos += increment * 5.0f;

        auto continueButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        continueButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        continueButton->AddChild<UITextElement>(font, "Continue", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        buttonPos += increment;

        auto settingsButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        settingsButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        settingsButton->AddChild<UITextElement>(font, "Settings", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        buttonPos += increment;

        auto backToMainButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        backToMainButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        backToMainButton->AddChild<UITextElement>(font, "Main Menu", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        pause->continueButton = continueButton;
        pause->backToMainButton = backToMainButton;
        pause->settingsButton = settingsButton;

        continueButton->navigationTargets.down = pause->settingsButton;
        continueButton->navigationTargets.up = pause->backToMainButton;

        settingsButton->navigationTargets.down = pause->backToMainButton;
        settingsButton->navigationTargets.up = pause->continueButton;

        backToMainButton->navigationTargets.down = pause->continueButton;
        backToMainButton->navigationTargets.up = pause->settingsButton;
    }

    pause->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return pause;
}