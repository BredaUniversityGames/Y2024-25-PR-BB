#pragma once
#include <engine.hpp>
#include <viewport.hpp>

class UIModule : public ModuleInterface
{
public:
    UIModule() = default;
    ~UIModule() override = default;

    NON_COPYABLE(UIModule);
    NON_MOVABLE(UIModule);

    std::unique_ptr<Viewport> _viewport;

    void CreateMainMenu();

private:
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) final;

    void Tick(MAYBE_UNUSED Engine& engine) final;
    void Shutdown(MAYBE_UNUSED Engine& engine) final { }
};
