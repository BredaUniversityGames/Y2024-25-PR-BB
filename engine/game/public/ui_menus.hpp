#pragma once
#include "canvas.hpp"

class UITextElement;
class UIProgressBar;
class GraphicsContext;
struct HUD
{
    std::weak_ptr<UIProgressBar> healthBar;
    std::weak_ptr<UIProgressBar> ultBar;
    std::weak_ptr<UIProgressBar> sprintBar;
    std::weak_ptr<UIProgressBar> grenadeBar;
    std::weak_ptr<UITextElement> ammoCounter;
};

std::pair<std::unique_ptr<Canvas>, HUD> HudCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);
void HudUpdate(HUD& hud, float timePassed);
