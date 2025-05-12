#pragma once
#include "canvas.hpp"
#include "fonts.hpp"

#include <array>
#include <ui_button.hpp>

class UIImage;
class UITextElement;
class UIProgressBar;
class GraphicsContext;
class ActionManager;

inline constexpr size_t MAX_DASH_CHARGE_COUNT = 3;

class HUD : public Canvas
{
public:
    static std::shared_ptr<HUD> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font);

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
    std::weak_ptr<UITextElement> ultReadyText;
    std::array<std::weak_ptr<UIImage>, MAX_DASH_CHARGE_COUNT> dashCharges;
};

class MainMenu : public Canvas
{
public:
    static std::shared_ptr<MainMenu> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font);

    MainMenu(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }

    std::weak_ptr<UIButton> playButton;
    std::weak_ptr<UIButton> settingsButton;
    std::weak_ptr<UIButton> controlsButton;
    std::weak_ptr<UIButton> quitButton;
    std::weak_ptr<UIButton> openLinkButton;
};

class GameVersionVisualization : public Canvas
{
public:
    static std::shared_ptr<GameVersionVisualization> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font, const std::string& text);

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
    static std::shared_ptr<LoadingScreen> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font);

    LoadingScreen(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }
};

class PauseMenu : public Canvas
{
public:
    static std::shared_ptr<PauseMenu> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font);

    PauseMenu(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }

    std::weak_ptr<UIButton> continueButton;
    std::weak_ptr<UIButton> settingsButton;
    std::weak_ptr<UIButton> controlsButton;
    std::weak_ptr<UIButton> backToMainButton;
};

class GameOverMenu : public Canvas
{
public:
    static std::shared_ptr<GameOverMenu> Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font);

    GameOverMenu(const glm::uvec2& screenResolution)
        : Canvas(screenResolution)
    {
    }

    std::weak_ptr<UIButton> continueButton;
    std::weak_ptr<UIButton> backToMainButton;
};

class ControlsMenu : public Canvas
{
public:
    static std::shared_ptr<ControlsMenu> Create(const glm::uvec2& screenResolution, GraphicsContext& graphicsContext, ActionManager& actionManager, std::shared_ptr<UIFont> font);

    ControlsMenu(const glm::uvec2& screenResolution, GraphicsContext& graphicsContext, ActionManager& actionManager, std::shared_ptr<UIFont> font)
        : Canvas(screenResolution), _graphicsContext(graphicsContext), _actionManager(actionManager), _font(font), _screenResolution(screenResolution)
    {
    }

    void UpdateBindings();

    std::shared_ptr<Canvas> actionsPanel;
    std::weak_ptr<UIButton> backButton;

    struct ActionControls
    {
        std::shared_ptr<Canvas> canvas;
        std::shared_ptr<UITextElement> nameText;

        struct Binding
        {
            std::shared_ptr<UITextElement> originName;
            std::shared_ptr<UIImage> glyph;
        };

        std::vector<Binding> bindings {};
    };

    struct ActionSetControls
    {
        std::shared_ptr<Canvas> canvas;
        std::shared_ptr<UITextElement> nameText;
        std::vector<ActionControls> actionControls;
    };

    std::vector<ActionSetControls> actionSetControls {};
    ResourceHandle<Sampler> sampler;

private:
    GraphicsContext& _graphicsContext;
    ActionManager& _actionManager;
    std::shared_ptr<UIFont> _font;
    const glm::uvec2 _screenResolution;

    std::unordered_map<std::string, ResourceHandle<GPUImage>> _glyphsCache {};

    ActionControls AddActionVisualization(const std::string& actionName, Canvas& parent, float positionY, bool isAnalogInput);
    ResourceHandle<GPUImage> GetGlyphImage(const std::string& path);
    void ClearBindings();
};
