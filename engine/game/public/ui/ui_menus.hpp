#pragma once
#include "canvas.hpp"
#include <array>
#include <ui_button.hpp>

class UIImage;
class UITextElement;
class UIProgressBar;
class GraphicsContext;

inline constexpr size_t MAX_DASH_CHARGE_COUNT = 3;

class HUD : public Canvas
{
public:
    static std::shared_ptr<HUD> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);

    HUD(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }

    std::weak_ptr<UIProgressBar> healthBar;
    std::weak_ptr<UIProgressBar> ultBar;
    std::weak_ptr<UIProgressBar> sprintBar;
    std::weak_ptr<UIProgressBar> grenadeBar;
    std::weak_ptr<UITextElement> ammoCounter;
    std::weak_ptr<UITextElement> scoreText;
    std::array<std::weak_ptr<UIImage>, MAX_DASH_CHARGE_COUNT> dashCharges;
};

class MainMenu : public Canvas
{
public:
    MainMenu(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);

    std::shared_ptr<UIButton> playButton;
    std::shared_ptr<UIButton> settingsButton;
    std::shared_ptr<UIButton> quitButton;
    std::shared_ptr<UIButton> openLinkButton;
};

MainMenu MainMenuCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);

class GameVersionVisualization : public Canvas
{
public:
    static std::shared_ptr<GameVersionVisualization> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, const std::string& text);

    GameVersionVisualization(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }
    std::shared_ptr<Canvas> canvas;
    std::weak_ptr<UITextElement> text;
};

GameVersionVisualization GameVersionVisualizationCreate(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, const std::string& text);
