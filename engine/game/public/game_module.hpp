#pragma once

#include "common.hpp"
#include "engine.hpp"
#include "game_settings.hpp"
#include "scene/model_loader.hpp"
#include "ui/ui_menus.hpp"

#include <stack>

constexpr const char* DISCORD_URL = "https://discord.gg/8RmgD2sz9M";

struct PlayerTag
{
};

struct EnemyTag
{
};

class GameModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Game Module"; }

    glm::ivec2 _lastMousePos {};

public:
    GameModule() = default;
    ~GameModule() override = default;

    void SetUIMenu(std::weak_ptr<Canvas> menu);
    void PushUIMenu(std::weak_ptr<Canvas> menu);
    void PopUIMenu();

    std::weak_ptr<UIElement> PopPreviousFocusedElement();
    void PushPreviousFocusedElement(std::weak_ptr<UIElement> element);

    GameSettings& GetSettings() { return gameSettings; };

    glm::vec4& GetFlashColor() { return flashColor; }

    std::optional<std::shared_ptr<MainMenu>> GetMainMenu();
    std::optional<std::shared_ptr<PauseMenu>> GetPauseMenu();
    std::optional<std::shared_ptr<HUD>> GetHUD();
    std::optional<std::shared_ptr<GameOverMenu>> GetGameOver();
    std::optional<std::shared_ptr<LoadingScreen>> GetLoadingScreen();

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);

    void ApplySettings(Engine& engine);
    void SetNextScene(const std::string& scriptFile);

    ModelLoader _modelsLoaded {};
    std::weak_ptr<MainMenu> _mainMenu;

private:
    void TransitionScene(Engine& engine);

    // UI

    std::stack<std::weak_ptr<Canvas>> _menuStack {};
    std::stack<std::weak_ptr<UIElement>> _focusedElementStack {};

    std::weak_ptr<HUD> _hud;
    std::weak_ptr<LoadingScreen> _loadingScreen;
    std::weak_ptr<PauseMenu> _pauseMenu;
    std::weak_ptr<GameOverMenu> _gameOver;
    std::weak_ptr<SettingsMenu> _settingsMenu;
    std::weak_ptr<ControlsMenu> _controlsMenu;
    std::weak_ptr<FrameCounter> _framerateCounter {};
    std::weak_ptr<CreditsMenu> _creditsMenu {};

    // Scene

    std::string _nextSceneToExecute {};

    // Settings

    GameSettings gameSettings {};

    // Gameplay elements
    glm::vec4 flashColor {};
};
