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

UIProgressBar::BarStyle LoadHealthBarStyle(GraphicsContext& graphicsContext)
{
    // common image data.
    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    UIProgressBar::BarStyle barStyle;
    barStyle.empty = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/health_empty.png"));
    barStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/health_full.png"));
    barStyle.fillStyle = UIProgressBar::BarStyle::FillStyle::eMask;
    return barStyle;
}

UIProgressBar::BarStyle LoadCircleBarStyle(GraphicsContext& graphicsContext)
{
    // common image data.
    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    UIProgressBar::BarStyle barStyle;
    barStyle.empty = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/circleempty.png"));
    barStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/circlefull.png"));
    barStyle.fillDirection = UIProgressBar::BarStyle::FillDirection::eFromBottom;
    barStyle.fillStyle = UIProgressBar::BarStyle::FillStyle::eMask;
    return barStyle;
}

UIProgressBar::BarStyle LoadUltBarStyle(GraphicsContext& graphicsContext)
{
    // common image data.
    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    UIProgressBar::BarStyle barStyle;
    barStyle.empty = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/ult_empty.png"));
    barStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/ult_full.png"));
    barStyle.fillDirection = UIProgressBar::BarStyle::FillDirection::eFromBottom;
    barStyle.fillStyle = UIProgressBar::BarStyle::FillStyle::eMask;
    return barStyle;
}

HUD HudCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution)
{

    HUD hud;
    // resource loading.

    auto font = LoadFromFile("assets/fonts/Rooters.ttf", 50, graphicsContext);

    UIProgressBar::BarStyle healtbarStyle
        = LoadHealthBarStyle(graphicsContext);
    UIProgressBar::BarStyle circleBarStyle = LoadCircleBarStyle(graphicsContext);
    UIProgressBar::BarStyle ultBarStyle = LoadUltBarStyle(graphicsContext);

    hud.canvas = std::make_unique<Canvas>(screenResolution);

    // temporary
    hud.canvas->SetAbsoluteTransform(hud.canvas->GetAbsoluteLocation(), hud.canvas->GetRelativeScale());

    CPUImage commonImageData {};
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;
    auto crosshair = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/cross_hair.png"));
    hud.canvas->AddChild<UIImage>(crosshair, glm::vec2(0), glm::vec2(120, 160) * 0.15f);

    hud.healthBar = hud.canvas->AddChild<UIProgressBar>(healtbarStyle, glm::vec2(0, 100), glm::vec2(700, 50));
    hud.healthBar.lock()->AddChild<UITextElement>(font, "health", 30);
    hud.healthBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomCenter;

    hud.ultBar
        = hud.canvas->AddChild<UIProgressBar>(ultBarStyle, glm::vec2(440, 250), glm::vec2(1920, 770) * 0.35f);
    hud.ultBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.ultBar.lock()->AddChild<UITextElement>(font, "ult", glm::vec2(-55, -20), 40);

    hud.sprintBar = hud.canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(200, 405), glm::vec2(100));
    hud.sprintBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.sprintBar.lock()->AddChild<UITextElement>(font, "sprint", 20);

    hud.grenadeBar = hud.canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(355, 335), glm::vec2(100));
    hud.grenadeBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.grenadeBar.lock()->AddChild<UITextElement>(font, "grenade", 20);

    hud.ammoCounter = hud.canvas->AddChild<UITextElement>(font, "5/8", glm::vec2(550, 100), 50);
    hud.ammoCounter.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud.scoreText = hud.canvas->AddChild<UITextElement>(font, "Score: 0", glm::vec2(100, 100), 50);
    hud.scoreText.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;

    // common image data.
    CPUImage imageData;
    imageData.format
        = vk::Format::eR8G8B8A8Unorm;
    imageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    imageData.isHDR = false;

    auto im = graphicsContext.Resources()->ImageResourceManager().Create(imageData.FromPNG("assets/textures/ui/gun.png"));

    auto gunPic = hud.canvas->AddChild<UIImage>(im, glm::vec2(460, 140), glm::vec2(720, 360) * 0.2f);
    gunPic.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.canvas->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return hud;
}

void HudUpdate(HUD& hud, float timePassed)
{

    // all temporary
    float fract = (sin(timePassed / 400.0f) + 1) * 0.5;
    if (auto locked = hud.healthBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(fract);
    }

    if (auto locked = hud.ultBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(fract);
    }

    if (auto locked = hud.sprintBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(0.9);
    }

    if (auto locked = hud.grenadeBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(0.3);
    }

    if (auto locked = hud.ammoCounter.lock(); locked != nullptr)
    {
        static float ammo = 8.0f;
        ammo -= 0.01;
        if (ammo <= 0.0f)
        {
            ammo = 8.0f;
        }

        locked->SetText(std::to_string(int(ammo)) + "/8");
    }
}

MainMenu MainMenuCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution)
{
    MainMenu mainMenu(screenResolution);
    // resource loading.
    auto loadButtonStyle = [&graphicsContext]()
    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format
            = vk::Format::eR8G8B8A8Unorm;
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

    // temporary
    mainMenu.SetAbsoluteTransform(mainMenu.GetAbsoluteLocation(), mainMenu.GetRelativeScale());

    constexpr float xMargin = 50;
    mainMenu.playButton = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(xMargin, 200), glm::vec2(878, 243) * .2f).lock();
    mainMenu.playButton.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    mainMenu.playButton.lock()->AddChild<UITextElement>(font, "play", 15).lock()->SetColor(glm::vec4(0, 0, 0, 1));

    mainMenu.settingsButton = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(xMargin, 250), glm::vec2(878, 243) * .2f).lock();
    mainMenu.settingsButton.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    mainMenu.settingsButton.lock()->AddChild<UITextElement>(font, "settings", 15).lock()->SetColor(glm::vec4(0, 0, 0, 1));

    mainMenu.quitButton = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(xMargin, 300), glm::vec2(878, 243) * .2f).lock();
    mainMenu.quitButton.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    mainMenu.quitButton.lock()->AddChild<UITextElement>(font, "quit", 15).lock()->SetColor(glm::vec4(0, 0, 0, 1));

    mainMenu.openLinkButton = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(200, 70), glm::vec2(878, 243) * .2f).lock();
    mainMenu.openLinkButton.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    mainMenu.openLinkButton.lock()->AddChild<UITextElement>(font, "discord", 15).lock()->SetColor(glm::vec4(0, 0, 0, 1));

    mainMenu.playButton.lock()->navigationTargets.down = mainMenu.settingsButton;
    mainMenu.playButton.lock()->navigationTargets.up = mainMenu.openLinkButton;

    mainMenu.settingsButton.lock()->navigationTargets.down = mainMenu.quitButton;
    mainMenu.settingsButton.lock()->navigationTargets.up = mainMenu.playButton;

    mainMenu.quitButton.lock()->navigationTargets.down = mainMenu.openLinkButton;
    mainMenu.quitButton.lock()->navigationTargets.up = mainMenu.settingsButton;

    mainMenu.openLinkButton.lock()->navigationTargets.down = mainMenu.playButton;
    mainMenu.openLinkButton.lock()->navigationTargets.up = mainMenu.quitButton;

    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    ResourceHandle<GPUImage> logo = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/blightspire_logo.png"));
    auto logoElement = mainMenu.AddChild<UIImage>(logo, glm::vec2(xMargin - 20, 100), glm::vec2(618, 217) * 0.4f);
    logoElement.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;

    mainMenu.UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return mainMenu;
}