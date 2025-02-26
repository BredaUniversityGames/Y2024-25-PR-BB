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
    constexpr float xMargin = 100;
    auto playButton = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(xMargin, 400), glm::vec2(878, 243) * .5f).lock();
    playButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    playButton->AddChild<UITextElement>(font, "play");

    auto settingsButton
        = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(xMargin, 500), glm::vec2(878, 243) * .5f).lock();
    settingsButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    settingsButton->AddChild<UITextElement>(font, "settings");

    auto openQuitModalButton = mainMenu.AddChild<UIButton>(buttonStyle, glm::vec2(xMargin, 600), glm::vec2(878, 243) * .5f).lock();
    openQuitModalButton->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    openQuitModalButton->AddChild<UITextElement>(font, "quit");

    playButton->navigationTargets.down = settingsButton;
    playButton->navigationTargets.up = openQuitModalButton;

    settingsButton->navigationTargets.down = openQuitModalButton;
    settingsButton->navigationTargets.up = playButton;

    openQuitModalButton->navigationTargets.down = playButton;
    openQuitModalButton->navigationTargets.up = settingsButton;

    mainMenu.playButton = playButton;
    mainMenu.UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return mainMenu;
}