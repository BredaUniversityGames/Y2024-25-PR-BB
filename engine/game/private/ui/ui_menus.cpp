#include "ui/ui_menus.hpp"
#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui_image.hpp"
#include "ui_progress_bar.hpp"
#include "ui_text.hpp"

#include <glm/glm.hpp>
#include <ui_button.hpp>

MainMenu::MainMenu(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution)
    : Canvas(screenResolution)
{
    anchorPoint = UIElement::AnchorPoint::eTopLeft;
    SetAbsoluteTransform(GetAbsoluteLocation(), screenResolution);

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

    auto font = LoadFromFile("assets/fonts/Rooters.ttf", 50, graphicsContext);

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

        auto logoElement = AddChild<UIImage>(logo, pos, size);
        logoElement->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    }

    // Discord Link
    {
        openLinkButton = AddChild<UIButton>(buttonStyle, glm::vec2(878, 243) * .4f, glm::vec2(878, 243) * .4f);
        openLinkButton->anchorPoint = UIElement::AnchorPoint::eBottomRight;
        openLinkButton->AddChild<UITextElement>(font, "check out our discord!", 20)->SetColor(glm::vec4(0, 0, 0, 1));
    }

    // Buttons

    auto buttonPanel = AddChild<Canvas>(glm::vec2 { 0.0f, 0.0f });

    {
        buttonPanel->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        buttonPanel->SetLocation(glm::vec2(screenResFloat.y * 0.1f, screenResFloat.y * 0.4f));
    }

    {
        glm::vec2 buttonPos = { 0.0f, 0.0f };
        constexpr glm::vec2 increment = { 0.0f, 90.0f };
        constexpr glm::vec2 buttonBaseSize = glm::vec2(878, 243) * 0.5f;
        constexpr float textSize = 50;

        playButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        playButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        playButton->AddChild<UITextElement>(font, "play", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        buttonPos += increment;

        settingsButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        settingsButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        settingsButton->AddChild<UITextElement>(font, "settings", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        buttonPos += increment;

        quitButton = buttonPanel->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
        quitButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        quitButton->AddChild<UITextElement>(font, "quit", textSize)->SetColor(glm::vec4(0, 0, 0, 1));

        playButton->navigationTargets.down = settingsButton;
        playButton->navigationTargets.up = openLinkButton;

        playButton->navigationTargets.down = settingsButton;
        playButton->navigationTargets.up = quitButton;

        settingsButton->navigationTargets.down = quitButton;
        settingsButton->navigationTargets.up = playButton;

        quitButton->navigationTargets.down = playButton;
        quitButton->navigationTargets.up = settingsButton;
        quitButton->navigationTargets.down = openLinkButton;
        quitButton->navigationTargets.up = settingsButton;

        openLinkButton->navigationTargets.down = playButton;
        openLinkButton->navigationTargets.up = quitButton;
    }

    UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();
}
