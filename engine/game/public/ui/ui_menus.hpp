#pragma once
#include "canvas.hpp"

#include <ui_button.hpp>

class UITextElement;
class UIProgressBar;
class GraphicsContext;
struct HUD
{
    std::shared_ptr<Canvas> canvas;
    std::weak_ptr<UIProgressBar> healthBar;
    std::weak_ptr<UIProgressBar> ultBar;
    std::weak_ptr<UIProgressBar> sprintBar;
    std::weak_ptr<UIProgressBar> grenadeBar;
    std::weak_ptr<UITextElement> ammoCounter;
};

HUD HudCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);
void HudUpdate(HUD& hud, float timePassed);

class MainMenu : public Canvas
{
public:
    MainMenu(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }
    std::weak_ptr<UIButton> playButton;
    std::weak_ptr<UIButton> settingsButton;
    std::weak_ptr<UIButton> quitButton;
};

MainMenu MainMenuCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);
