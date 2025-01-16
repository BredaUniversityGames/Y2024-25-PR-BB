#pragma once
#include <canvas.hpp>

class UIProgressBar;
class GraphicsContext;
struct HUD
{
    std::weak_ptr<UIProgressBar> healthBar;
    std::weak_ptr<UIProgressBar> ultBar;
    std::weak_ptr<UIProgressBar> sprintBar;
    std::weak_ptr<UIProgressBar> grenadeBar;
    ;
};

std::pair<std::unique_ptr<Canvas>, HUD> CreateHud(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);
