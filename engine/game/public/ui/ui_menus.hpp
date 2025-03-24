#pragma once
#include "canvas.hpp"
#include <array>
#include <ui_button.hpp>

class UIImage;
class UITextElement;
class UIProgressBar;
class GraphicsContext;

inline constexpr size_t MAX_DASH_CHARGE_COUNT = 3;

struct HUD
{
    std::shared_ptr<Canvas> canvas;
    std::weak_ptr<UIProgressBar> healthBar;
    std::weak_ptr<UIProgressBar> ultBar;
    std::weak_ptr<UIProgressBar> sprintBar;
    std::weak_ptr<UIProgressBar> grenadeBar;
    std::weak_ptr<UITextElement> ammoCounter;
    std::weak_ptr<UITextElement> scoreText;
    std::array<std::weak_ptr<UIImage>, MAX_DASH_CHARGE_COUNT> dashCharges;
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

struct GameVersionVisualization
{
    std::shared_ptr<Canvas> canvas;
    std::weak_ptr<UITextElement> text;
};

GameVersionVisualization GameVersionVisualizationCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::string_view text);
