#pragma once

#include "common.hpp"
#include "engine.hpp"

struct PlayerTag
{
};

class GameModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Game Module"; }

public:
    GameModule() = default;
    ~GameModule() override = default;

    NON_COPYABLE(GameModule);
    NON_MOVABLE(GameModule);
};