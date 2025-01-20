#include "ui_menus.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui_progress_bar.hpp"

#include <fonts.hpp>
#include <glm/glm.hpp>
#include <ui_image.hpp>
#include <ui_text.hpp>
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

std::pair<std::unique_ptr<Canvas>, HUD> CreateHud(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution)
{

    HUD hud;
    // resource loading.

    auto font = LoadFromFile("assets/fonts/Rooters.ttf", 50, graphicsContext);

    UIProgressBar::BarStyle healtbarStyle
        = LoadHealthBarStyle(graphicsContext);
    UIProgressBar::BarStyle circleBarStyle = LoadCircleBarStyle(graphicsContext);

    std::unique_ptr<Canvas> canvas = std::make_unique<Canvas>(screenResolution);

    // temporary
    canvas->SetAbsoluteTransform(canvas->GetAbsoluteLocation(), canvas->GetRelativeScale());

    hud.healthBar = canvas->AddChild<UIProgressBar>(healtbarStyle, glm::vec2(0, 700), glm::vec2(1000, 50));
    hud.ultBar = canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(300, 300), glm::vec2(250));
    hud.ultBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud.sprintBar = canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(225, 430), glm::vec2(100));
    hud.sprintBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud.grenadeBar = canvas->AddChild<UIProgressBar>(circleBarStyle, glm::vec2(380, 360), glm::vec2(100));
    hud.grenadeBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud.ammoCounter = canvas->AddChild<UITextElement>(font, "5/8", glm::vec2(550, 150), 50);
    hud.ammoCounter.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    // common image data.
    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    auto im = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/gun.png"));

    auto gunPic = canvas->AddChild<UIImage>(im, glm::vec2(460, 200), glm::vec2(720, 360) * 0.2f);
    gunPic.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;
    canvas->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return { std::move(canvas), hud };
}