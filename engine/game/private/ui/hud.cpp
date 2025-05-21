#include "ui/ui_menus.hpp"

#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui_image.hpp"
#include "ui_progress_bar.hpp"
#include "ui_text.hpp"

#include <resource_management/sampler_resource_manager.hpp>

UIProgressBar::BarStyle LoadHealthBarStyle(GraphicsContext& graphicsContext, auto sampler)
{
    // common image data.
    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    UIProgressBar::BarStyle barStyle;
    barStyle.empty = ResourceHandle<GPUImage>::Null();
    barStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/health_bar.png"), sampler);
    barStyle.fillStyle = UIProgressBar::BarStyle::FillStyle::eMask;
    barStyle.fillDirection = UIProgressBar::BarStyle::FillDirection::eLeftToRight;
    return barStyle;
}

std::shared_ptr<HUD> HUD::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{

    std::shared_ptr<HUD> hud = std::make_shared<HUD>(screenResolution);

    CPUImage commonImageData {};
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    SamplerCreation samplerData {};

    samplerData.minFilter = vk::Filter::eNearest;
    samplerData.magFilter = vk::Filter::eNearest;
    ResourceHandle<Sampler> HUDSampler = graphicsContext.Resources()->SamplerResourceManager().Create(samplerData);

    ImageResourceManager& imageManager = graphicsContext.Resources()->ImageResourceManager();

    hud->SetAbsoluteTransform(hud->GetAbsoluteLocation(), screenResolution);

    ResourceHandle<GPUImage> crosshairImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/cross_hair.png"));
    hud->AddChild<UIImage>(crosshairImage, glm::vec2(0, 7), glm::vec2(25, 42) * 2.0f);

    // stats bg
    ResourceHandle<GPUImage> statsBGImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/stats_bg.png"), HUDSampler);
    auto statsBG = hud->AddChild<UIImage>(statsBGImage, glm::vec2(0, 0), glm::vec2(113, 39) * 8.0f);
    statsBG->anchorPoint = UIElement::AnchorPoint::eBottomLeft;

    // gun
    ResourceHandle<GPUImage> gunBGImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/gun_bg.png"), HUDSampler);
    auto gunBG = hud->AddChild<UIImage>(gunBGImage, glm::vec2(0, 0), glm::vec2(69, 35) * 8.0f);
    gunBG->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    ResourceHandle<GPUImage> gunImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/gun.png"), HUDSampler);
    auto gun = hud->AddChild<UIImage>(gunImage, glm::vec2(16, 12) * 8.0f, glm::vec2(19, 8) * 8.0f);
    gun->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    hud->ammoCounter = hud->AddChild<UITextElement>(font, "17", glm::vec2(52, 10) * 8.0f, 12 * 8.0);
    hud->ammoCounter.lock()->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    // hitmarker
    auto hitmarkerImage
        = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/hitmarker.png"));
    auto hitmarkerCritImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/hitmarker_crit.png"));

    hud->hitmarker = hud->AddChild<UIImage>(hitmarkerImage, glm::vec2(0, 7), glm::vec2(25, 42) * 2.0f);
    hud->hitmarker.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;

    hud->hitmarkerCrit = hud->AddChild<UIImage>(hitmarkerCritImage, glm::vec2(0, 7), glm::vec2(25, 42) * 2.0f);
    hud->hitmarkerCrit.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;

    // dashes
    ResourceHandle<GPUImage>
        dashChargeImage
        = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/dash_charge.png"), HUDSampler);

    for (int32_t i = 0; i < static_cast<int32_t>(hud->dashCharges.size()); i++)
    {
        hud->dashCharges[i] = hud->AddChild<UIImage>(dashChargeImage, (glm::vec2(21 + 9.f * i, 19) * 8.0f), glm::vec2(6.f) * 8.f);
        hud->dashCharges[i].lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    }

    // healthbar
    UIProgressBar::BarStyle healthBarStyle = LoadHealthBarStyle(graphicsContext, HUDSampler);
    hud->healthBar = hud->AddChild<UIProgressBar>(healthBarStyle, glm::vec2(10, 11) * 8.0f, glm::vec2(94, 5) * 8.0f);
    hud->healthBar.lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;

    // souls indicator
    ResourceHandle<GPUImage> soulImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/souls_indicator.png"), HUDSampler);
    hud->soulIndicator = hud->AddChild<UIImage>(soulImage, glm::vec2(11, 18) * 8.0f, glm::vec2(5, 8) * 8.0f);
    hud->soulIndicator.lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;

    ResourceHandle<GPUImage> waveBGImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/wave_bg.png"), HUDSampler);
    hud->AddChild<UIImage>(waveBGImage, glm::vec2(0, 0) * 8.0f, glm::vec2(59, 54) * 8.0f)->anchorPoint = UIElement::AnchorPoint::eTopRight;

    ResourceHandle<GPUImage> coinImage = imageManager.Create(commonImageData.FromPNG("assets/textures/ui/coin.png"), HUDSampler);
    hud->AddChild<UIImage>(coinImage, glm::vec2(49, 20) * 8.0f + glm::vec2(300.f, 0), glm::vec2(4 * 8))->anchorPoint = UIElement::AnchorPoint::eBottomLeft;

    // wave counter

    hud->waveCounterText = hud->AddChild<UITextElement>(font, "0", glm::vec2(140, 26), 20 * 8.0);
    hud->waveCounterText.lock()->anchorPoint = UIElement::AnchorPoint::eTopRight;
    hud->waveCounterText.lock()->display_color = glm::vec4(0.459 * 2.0, 0.31 * 2.0, 0.28 * 2.0, 1);

    hud->scoreText
        = hud->AddChild<UITextElement>(font, "220", glm::vec2(445 + 300, 142), 10 * 8.0);
    hud->scoreText.lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    // hud->scoreText.lock()->display_color = glm::vec4(0.9, 0.8, 0.2, 1);

    hud->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();
    return hud;
}