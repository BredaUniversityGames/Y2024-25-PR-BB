#include "ui/ui_menus.hpp"

#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui_image.hpp"
#include "ui_progress_bar.hpp"
#include "ui_text.hpp"

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

std::shared_ptr<HUD> HUD::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{

    std::shared_ptr<HUD> hud = std::make_shared<HUD>(screenResolution);
    // resource loading.

    const UIProgressBar::BarStyle healtbarStyle
        = LoadHealthBarStyle(graphicsContext);
    const UIProgressBar::BarStyle circleBarStyle = LoadCircleBarStyle(graphicsContext);
    const UIProgressBar::BarStyle ultBarStyle = LoadUltBarStyle(graphicsContext);

    hud->SetAbsoluteTransform(hud->GetAbsoluteLocation(), screenResolution);

    CPUImage commonImageData {};
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;
    auto crosshair = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/cross_hair.png"));
    hud->AddChild<UIImage>(crosshair, glm::vec2(0, 7), glm::vec2(25, 42) * 2.0f);

    hud->healthBar = hud->AddChild<UIProgressBar>(healtbarStyle, glm::vec2(0, 100), glm::vec2(700, 50));
    hud->healthBar.lock()->AddChild<UITextElement>(font, "health", 50);
    hud->healthBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomCenter;

    hud->ultBar = hud->AddChild<UIProgressBar>(ultBarStyle, glm::vec2(440, 250), glm::vec2(1920, 770) * 0.35f);
    hud->ultBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud->ultBar.lock()->AddChild<UITextElement>(font, "ult", glm::vec2(-55, -20), 50);

    hud->sprintBar = hud->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(200, 405), glm::vec2(100));
    hud->sprintBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud->sprintBar.lock()->AddChild<UITextElement>(font, "sprint", 30);

    hud->grenadeBar = hud->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(355, 335), glm::vec2(100));
    hud->grenadeBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    hud->grenadeBar.lock()->AddChild<UITextElement>(font, "grenade", 30);

    hud->ammoCounter = hud->AddChild<UITextElement>(font, "6/6", glm::vec2(550, 100), 80);
    hud->ammoCounter.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud->scoreText = hud->AddChild<UITextElement>(font, "Score: 0", glm::vec2(100, 100), 100);
    hud->scoreText.lock()->anchorPoint = UIElement::AnchorPoint::eTopLeft;

    hud->multiplierText = hud->AddChild<UITextElement>(font, "1.0x", glm::vec2(150, 600), 100);
    hud->multiplierText.lock()->anchorPoint = UIElement::AnchorPoint::eTopRight;

    hud->ultReadyText = hud->AddChild<UITextElement>(font, "", glm::vec2(0, 135), 50);
    hud->ultReadyText.lock()->anchorPoint = UIElement::AnchorPoint::eBottomCenter;

    // common image data.
    CPUImage imageData;
    imageData.format
        = vk::Format::eR8G8B8A8Unorm;
    imageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    imageData.isHDR = false;

    auto im = graphicsContext.Resources()->ImageResourceManager().Create(imageData.FromPNG("assets/textures/ui/gun.png"));

    auto hitmarkerImage = graphicsContext.Resources()->ImageResourceManager().Create(imageData.FromPNG("assets/textures/ui/hitmarker.png"));

    auto gunPic = hud->AddChild<UIImage>(im, glm::vec2(460, 140), glm::vec2(720, 360) * 0.2f);

    hud->hitmarker = hud->AddChild<UIImage>(hitmarkerImage, glm::vec2(0, 7), glm::vec2(25, 42) * 2.0f);
    hud->hitmarker.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    gunPic->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    auto dashCircle = graphicsContext.Resources()->ImageResourceManager().Create(imageData.FromPNG("assets/textures/ui/grey_ellipse.png"));

    for (int32_t i = 0; i < static_cast<int32_t>(hud->dashCharges.size()); i++)
    {
        hud->dashCharges[i] = hud->AddChild<UIImage>(dashCircle, glm::vec2((i * 60) + 100, 100), glm::vec2(50));
        hud->dashCharges[i].lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    }

    hud->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return hud;
}