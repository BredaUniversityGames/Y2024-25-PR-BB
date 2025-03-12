#pragma once

#include "common.hpp"
#include "engine.hpp"
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

    glm::ivec2 _lastMousePos {};
    bool _updateHud = false;

public:
    GameModule() = default;
    ~GameModule() override = default;

    HUD _hud;

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);
};