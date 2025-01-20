#pragma once
#include "engine.hpp"
#include "ui_input.hpp"
#include "viewport.hpp"
class GraphicsContext;
class UIModule : public ModuleInterface
{
public:
    UIModule() = default;
    ~UIModule() override = default;

    std::string_view GetName() override { return "UI Module"; }

    NON_COPYABLE(UIModule);
    NON_MOVABLE(UIModule);

    NO_DISCARD Viewport& GetViewport() { return *_viewport; };
    NO_DISCARD const Viewport& GetViewport() const { return *_viewport; };

private:
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) final;

    UIInputContext _uiInputContext;
    std::unique_ptr<Viewport> _viewport;
    std::shared_ptr<GraphicsContext> _graphicsContext;

    void Tick(MAYBE_UNUSED Engine& engine) final;
    void Shutdown(MAYBE_UNUSED Engine& engine) final { }
};
