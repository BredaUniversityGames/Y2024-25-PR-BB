#pragma once

#include "common.hpp"
#include "engine.hpp"
#include "scene/model_loader.hpp"
#include "ui/ui_menus.hpp"

inline const std::string DISCORD_URL = "https://discord.gg/8RmgD2sz9M";

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

    void SetMainMenuEnabled(bool val);
    void SetHUDEnabled(bool val);
    void SetLoadingScreenEnabled(bool val);
    void SetPauseMenuEnabled(bool val);
    void SetGameOverMenuEnabled(bool val);

    std::optional<std::shared_ptr<MainMenu>> GetMainMenu()
    {
        if (auto lock = _mainMenu.lock())
        {
            return lock;
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<PauseMenu>> GetPauseMenu()
    {
        if (auto lock = _pauseMenu.lock())

        {
            return lock;
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<HUD>> GetHUD()
    {
        if (auto lock = _hud.lock())
        {
            return lock;
        }
        return std::nullopt;
    }

    std::optional<std::shared_ptr<GameOverMenu>> GetGameOver()
    {
        if (auto lock = _gameOver.lock())
        {
            return lock;
        }
        return std::nullopt;
    }

    std::weak_ptr<HUD> _hud;
    std::weak_ptr<MainMenu> _mainMenu;
    std::weak_ptr<LoadingScreen> _loadingScreen;
    std::weak_ptr<PauseMenu> _pauseMenu;
    std::weak_ptr<GameOverMenu> _gameOver;

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);

    void TransitionScene(const std::string& scriptFile);

    ModelLoader _modelsLoaded {};

    bool _updateHud = false;
    std::string _nextSceneToExecute {};
};
