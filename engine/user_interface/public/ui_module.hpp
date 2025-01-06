#pragma once
#include "engine.hpp"
#include "viewport.hpp"

struct InputManagers
{
    InputDeviceManager& inputDeviceManager;
    ActionManager& actionManager;
};

// Note: currently key navigation only works for controller.
class UIInputContext
{
public:
    // Returns if the UI input has been consumed this frame, can be either mouse or controller.
    bool HasInputBeenConsumed() const { return _hasInputBeenConsumed; }
    bool GamepadHasFocus() const { return _gamepadHasFocus; }

    // Consume the UI input for this frame.
    void
    ConsumeInput()
    {
        _hasInputBeenConsumed = true;
    }

    std::weak_ptr<UIElement> focusedUIElement = {};

    UINavigationMappings::Direction GetDirection(const ActionManager& actionManager);

private:
    friend class UIModule;

    bool _gamepadHasFocus = false;

    // If the input has been consumed this frame.
    bool _hasInputBeenConsumed = false;

    std::string _navigationActionName = "Look";
    UINavigationMappings::Direction _previousNavigationDirection {};
};

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

    void CreateMainMenu(std::function<void()> onPlayButtonClick, std::function<void()> onExitButtonClick);

private:
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) final;

    UIInputContext _uiInputContext;
    std::unique_ptr<Viewport> _viewport;
    std::shared_ptr<GraphicsContext> _graphicsContext;

    void Tick(MAYBE_UNUSED Engine& engine) final;
    void Shutdown(MAYBE_UNUSED Engine& engine) final { }
};
