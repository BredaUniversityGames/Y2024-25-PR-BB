#pragma once
#include "canvas.hpp"

class UIImage;
class UITextElement;
class UIProgressBar;
class GraphicsContext;

inline constexpr size_t MAX_DASH_CHARGE_COUNT = 3;

struct HUD
{
    std::weak_ptr<UIProgressBar> healthBar;
    std::weak_ptr<UIProgressBar> ultBar;
    std::weak_ptr<UIProgressBar> sprintBar;
    std::weak_ptr<UIProgressBar> grenadeBar;
    std::weak_ptr<UITextElement> ammoCounter;
    std::weak_ptr<UITextElement> scoreText;
    std::array<std::weak_ptr<UIImage>, MAX_DASH_CHARGE_COUNT> dashCharges;
};

std::pair<std::unique_ptr<Canvas>, HUD> HudCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);
void HudUpdate(HUD& hud, float timePassed);
