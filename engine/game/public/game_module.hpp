#pragma once

#include "common.hpp"
#include "engine.hpp"
#include "ui/ui_menus.hpp"

struct PlayerTag
{
};

class GameModule : public ModuleInterface
{
public:
    GameModule() = default;
    ~GameModule() override = default;

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);

    ModuleTickOrder Init(Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Game Module"; }

    void TransitionScene(const std::string& scriptFile);

    HUD _hud;
    glm::ivec2 _lastMousePos {};
    bool _updateHud = false;
    std::string _nextScene {};
};