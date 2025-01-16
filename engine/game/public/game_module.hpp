#pragma once

#include "common.hpp"
#include "engine.hpp"

#include <ui_menus.hpp>

class GameModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Game Module"; }

    HUD _hud;

public:
    GameModule() = default;
    ~GameModule() override = default;

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);
};