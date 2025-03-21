#include "ui/ui_menus.hpp"
#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui_image.hpp"
#include "ui_progress_bar.hpp"
#include "ui_text.hpp"
#include <glm/glm.hpp>

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

std::pair<std::unique_ptr<Canvas>, HUD> HudCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution)
{

    HUD hud;
    // resource loading.

    auto font = LoadFromFile("assets/fonts/Rooters.ttf", 50, graphicsContext);

    UIProgressBar::BarStyle healtbarStyle
        = LoadHealthBarStyle(graphicsContext);
    UIProgressBar::BarStyle circleBarStyle = LoadCircleBarStyle(graphicsContext);
    UIProgressBar::BarStyle ultBarStyle = LoadUltBarStyle(graphicsContext);

    std::unique_ptr<Canvas> canvas = std::make_unique<Canvas>(screenResolution);

    // temporary
    canvas->SetAbsoluteTransform(canvas->GetAbsoluteLocation(), screenResolution);

    CPUImage commonImageData {};
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;
    auto crosshair = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/cross_hair.png"));
    canvas->AddChild<UIImage>(crosshair, glm::vec2(0), glm::vec2(120, 160) * 0.15f);

    hud.healthBar = canvas->AddChild<UIProgressBar>(healtbarStyle, glm::vec2(0, 100), glm::vec2(700, 50));
    hud.healthBar.lock()->AddChild<UITextElement>(font, "health", 30);
    hud.healthBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomCenter;

    hud.ultBar
        = canvas->AddChild<UIProgressBar>(ultBarStyle, glm::vec2(440, 250), glm::vec2(1920, 770) * 0.35f);
    hud.ultBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.ultBar.lock()->AddChild<UITextElement>(font, "ult", glm::vec2(-55, -20), 40);

    hud.sprintBar = canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(200, 405), glm::vec2(100));
    hud.sprintBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.sprintBar.lock()->AddChild<UITextElement>(font, "sprint", 20);

    hud.grenadeBar = canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(355, 335), glm::vec2(100));
    hud.grenadeBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud.grenadeBar.lock()->AddChild<UITextElement>(font, "grenade", 20);

    hud.ammoCounter = canvas->AddChild<UITextElement>(font, "5/8", glm::vec2(550, 100), 50);
    hud.ammoCounter.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud.scoreText = canvas->AddChild<UITextElement>(font, "Score: 0", glm::vec2(100, 100), 50);
    hud.scoreText.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;

    // common image data.
    CPUImage imageData;
    imageData.format
        = vk::Format::eR8G8B8A8Unorm;
    imageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    imageData.isHDR = false;

    auto im = graphicsContext.Resources()->ImageResourceManager().Create(imageData.FromPNG("assets/textures/ui/gun.png"));

    auto gunPic = canvas->AddChild<UIImage>(im, glm::vec2(460, 140), glm::vec2(720, 360) * 0.2f);
    gunPic.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    auto dashCircle = graphicsContext.Resources()->ImageResourceManager().Create(imageData.FromPNG("assets/textures/ui/grey_ellipse.png"));

    for (int32_t i = 0; i < static_cast<int32_t>(hud.dashCharges.size()); i++)
    {
        hud.dashCharges[i] = canvas->AddChild<UIImage>(dashCircle, glm::vec2((i * 60) + 100, 100), glm::vec2(50));
        hud.dashCharges[i].lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    }

    canvas->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return { std::move(canvas), hud };
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