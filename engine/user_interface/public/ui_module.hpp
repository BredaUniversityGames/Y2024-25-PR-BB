#pragma once
#include "engine.hpp"
#include "viewport.hpp"

class GraphicsContext;
class UIModule : public ModuleInterface
{
public:
    UIModule() = default;
    ~UIModule() override = default;

    NON_COPYABLE(UIModule);
    NON_MOVABLE(UIModule);

    NO_DISCARD Viewport& GetViewport() { return *_viewport; };
    NO_DISCARD const Viewport& GetViewport() const { return *_viewport; };

    void CreateMainMenu(std::shared_ptr<GraphicsContext> context, std::function<void(void)> onPlayButtonClick, std::function<void(void)> onExitButtonClick);

private:
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) final;

    std::unique_ptr<Viewport> _viewport;

    void Tick(MAYBE_UNUSED Engine& engine) final;
    void Shutdown(MAYBE_UNUSED Engine& engine) final { }
};
