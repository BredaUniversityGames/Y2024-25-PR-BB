#pragma once

#include "common.hpp"
#include "engine.hpp"
#include "scene/model_loader.hpp"
#include "ui/ui_menus.hpp"

struct PlayerTag
{
};

class GameModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Game Module"; }

    std::weak_ptr<MainMenu> _mainMenu;

    glm::ivec2 _lastMousePos {};

public:
    GameModule() = default;
    ~GameModule() override = default;

    void SetMainMenuEnabled(bool val);
    void SetHUDEnabled(bool val);
    MainMenu& GetMainMenu() const { return *_mainMenu.lock(); }

    HUD _hud;
    GameVersionVisualization _gameVersionVisualization;

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);

    void TransitionScene(const std::string& scriptFile);

    ModelLoader _modelsLoaded {};

    bool _updateHud = false;
    std::string _nextSceneToExecute {};
};
