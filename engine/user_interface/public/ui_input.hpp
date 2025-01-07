#pragma once
#include "ui_navigation_mappings.hpp"

class UIElement;

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

    UINavigationDirection GetDirection(const ActionManager& actionManager);

private:
    friend class UIModule;

    bool _gamepadHasFocus = false;

    // If the input has been consumed this frame.
    bool _hasInputBeenConsumed = false;

    std::string _navigationActionName = "Look";
    UINavigationDirection _previousNavigationDirection {};
};

inline bool IsMouseInsideBoundary(const glm::vec2& mousePos, const glm::vec2& location, const glm::vec2& scale)
{
    return mousePos.x > location.x
        && mousePos.x < location.x + scale.x
        && mousePos.y > location.y
        && mousePos.y < location.y + scale.y;
}
