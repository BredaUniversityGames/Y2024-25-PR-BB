#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "ui/ui_menus.hpp"
#include "ui_button.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include <glm/glm.hpp>

std::shared_ptr<PauseMenu> PauseMenu::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{
    auto pause = std::make_shared<PauseMenu>(screenResolution);
    pause->anchorPoint = UIElement::AnchorPoint::eMiddle;
    pause->SetAbsoluteTransform(pause->GetAbsoluteLocation(), screenResolution);

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

        buttonStyle.normalImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button.png"), sampler);
        buttonStyle.hoveredImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_2.png"), sampler);
        buttonStyle.pressedImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_selected.png"), sampler);
    }

    auto buttonPanel = pause->AddChild<Canvas>(glm::vec2 { 0.0f, 0.0f });

    {
        buttonPanel->anchorPoint = UIElement::AnchorPoint::eMiddle;
        // buttonPanel->SetLocation(glm::vec2(screenResFloat.y * 0.1f, screenResFloat.y * 0.4f));
    }

    {
        glm::vec2 buttonPos = { 0.0f, -300.0f };
        constexpr glm::vec2 increment = { 0.0f, 140.0f };
        constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 6.0f;
        constexpr float textSize = 60;

        buttonPanel->AddChild<UITextElement>(font, "Paused", buttonPos, 200);
        buttonPos += increment * 2.0f;

        auto continueButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        continueButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        continueButton->AddChild<UITextElement>(font, "Continue", textSize)->SetColor(glm::vec4(1, 1, 1, 1));

        buttonPos += increment;

        auto controlsButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        controlsButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        controlsButton->AddChild<UITextElement>(font, "Controls", textSize)->SetColor(glm::vec4(1, 1, 1, 1));

        buttonPos += increment;

        auto settingsButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        settingsButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        settingsButton->AddChild<UITextElement>(font, "Settings", textSize)->SetColor(glm::vec4(1, 1, 1, 1));

        buttonPos += increment;

        auto backToMainButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        backToMainButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        backToMainButton->AddChild<UITextElement>(font, "Main Menu", textSize)->SetColor(glm::vec4(1, 1, 1, 1));

        pause->continueButton = continueButton;
        pause->backToMainButton = backToMainButton;
        pause->settingsButton = settingsButton;
        pause->controlsButton = controlsButton;

        continueButton->navigationTargets.down = pause->controlsButton;
        continueButton->navigationTargets.up = pause->backToMainButton;

        controlsButton->navigationTargets.down = pause->settingsButton;
        controlsButton->navigationTargets.up = pause->continueButton;

        settingsButton->navigationTargets.down = pause->backToMainButton;
        settingsButton->navigationTargets.up = pause->controlsButton;

        backToMainButton->navigationTargets.down = pause->continueButton;
        backToMainButton->navigationTargets.up = pause->settingsButton;
    }

    pause->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return pause;
}
