#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui/ui_menus.hpp"
#include "ui_button.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include <glm/glm.hpp>

std::shared_ptr<MainMenu> MainMenu::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{
    auto main = std::make_shared<MainMenu>(screenResolution);

    main->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    main->SetAbsoluteTransform(main->GetAbsoluteLocation(), screenResolution);

    // resource loading.
    auto loadButtonStyle = [&graphicsContext]()
    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        UIButton::ButtonStyle buttonStyle {
            .normalImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_st.png")),
            .hoveredImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_h.png")),
            .pressedImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_p.png"))
        };
        return buttonStyle;
    };

    UIButton::ButtonStyle buttonStyle = loadButtonStyle();
    glm::vec2 screenResFloat = { 1920, 1080 };

    // Title
    {
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.FromPNG("assets/textures/blightspire_logo.png");

        glm::vec2 pos = glm::vec2(screenResFloat.y * 0.05f);
        glm::vec2 size = glm::vec2(((float)commonImageData.width / (float)commonImageData.height), 1.0f) * (0.25f * screenResFloat.y);

        ResourceHandle<GPUImage> logo = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData);

        auto logoElement = main->AddChild<UIImage>(logo, pos, size);
        logoElement->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    }

    // Buttons

    auto buttonPanel = main->AddChild<Canvas>(glm::vec2 { 0.0f, 0.0f });

    {
        buttonPanel->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        buttonPanel->SetLocation(glm::vec2(screenResFloat.y * 0.1f, screenResFloat.y * 0.4f));
    }

    {
        glm::vec2 buttonPos = { 0.0f, 0.0f };
        constexpr glm::vec2 increment = { 0.0f, 90.0f };
        constexpr glm::vec2 buttonBaseSize = glm::vec2(878, 243) * 0.5f;
        constexpr float textSize = 50;

        auto openLinkButton = main->AddChild<UIButton>(buttonStyle, buttonBaseSize * 1.4f, buttonBaseSize * 1.4f);
        openLinkButton->anchorPoint = UIElement::AnchorPoint::eBottomRight;
        openLinkButton->AddChild<UITextElement>(font, "CHECK OUT OUR DISCORD!", 40)->SetColor(glm::vec4(0, 0, 0, 1));

        main->openLinkButton = openLinkButton;

        auto playButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        playButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        playButton->AddChild<UITextElement>(font, "PLAY", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        buttonPos += increment;

        auto settingsButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        settingsButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        settingsButton->AddChild<UITextElement>(font, "SETTINGS", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        buttonPos += increment;

        auto quitButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        quitButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        quitButton->AddChild<UITextElement>(font, "QUIT", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        main->playButton = playButton;
        main->quitButton = quitButton;
        main->settingsButton = settingsButton;

        playButton->navigationTargets.down = main->settingsButton;
        playButton->navigationTargets.up = main->openLinkButton;

        settingsButton->navigationTargets.down = main->quitButton;
        settingsButton->navigationTargets.up = main->playButton;

        quitButton->navigationTargets.down = main->openLinkButton;
        quitButton->navigationTargets.up = main->settingsButton;

        main->openLinkButton.lock()->navigationTargets.down = main->playButton;
        main->openLinkButton.lock()->navigationTargets.up = main->quitButton;
    }

    main->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return main;
}
