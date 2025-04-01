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
    std::weak_ptr<UITextElement> multiplierText;
    std::array<std::weak_ptr<UIImage>, MAX_DASH_CHARGE_COUNT> dashCharges;
};

class MainMenu : public Canvas
{
public:
    static std::shared_ptr<MainMenu> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);

    MainMenu(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }

    std::weak_ptr<UIButton> playButton;
    std::weak_ptr<UIButton> settingsButton;
    std::weak_ptr<UIButton> quitButton;
    std::weak_ptr<UIButton> openLinkButton;
};

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

class LoadingScreen : public Canvas
{
public:
    static std::shared_ptr<LoadingScreen> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);

    LoadingScreen(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }
};

class PauseMenu : public Canvas
{
public:
    static std::shared_ptr<PauseMenu> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution);

    PauseMenu(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }

    std::weak_ptr<UIButton> continueButton;
    std::weak_ptr<UIButton> settingsButton;
    std::weak_ptr<UIButton> backToMainButton;
};