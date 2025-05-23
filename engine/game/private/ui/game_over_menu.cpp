#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"

std::shared_ptr<GameOverMenu> GameOverMenu::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{
    auto over = std::make_shared<GameOverMenu>(screenResolution);
    over->anchorPoint = UIElement::AnchorPoint::eMiddle;
    over->SetAbsoluteTransform(over->GetAbsoluteLocation(), screenResolution);

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
        auto image = over->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
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

    auto buttonPanel = over->AddChild<Canvas>(glm::vec2 { 0.0f, 0.0f });

    {
        buttonPanel->anchorPoint = UIElement::AnchorPoint::eMiddle;
        // buttonPanel->SetLocation(glm::vec2(screenResFloat.y * 0.1f, screenResFloat.y * 0.4f));
    }

    {
        glm::vec2 buttonPos = { 0.0f, -300.0f };
        constexpr glm::vec2 increment = { 0.0f, 140.0f };
        constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 6.0f;
        constexpr float textSize = 60;

        buttonPanel->AddChild<UITextElement>(font, "Game Over!", buttonPos, 200);
        buttonPos += increment * 2.0f;

        auto continueButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        continueButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        continueButton->AddChild<UITextElement>(font, "Retry", textSize);

        buttonPos += increment;

        auto backToMainButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        backToMainButton->anchorPoint = UIElement::AnchorPoint::eMiddle;
        backToMainButton->AddChild<UITextElement>(font, "Main Menu", textSize);

        over->continueButton = continueButton;
        over->backToMainButton = backToMainButton;

        continueButton->navigationTargets.down = over->backToMainButton;
        continueButton->navigationTargets.up = over->backToMainButton;

        backToMainButton->navigationTargets.down = over->continueButton;
        backToMainButton->navigationTargets.up = over->continueButton;
    }

    over->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return over;
}