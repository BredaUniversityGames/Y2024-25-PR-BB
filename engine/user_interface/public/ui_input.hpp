#pragma once
#include "ui_navigation.hpp"

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
    float _inputDeadZone = 0.2f;
    std::string _navigationActionName = "Look";
    UINavigationDirection _previousNavigationDirection {};
};

/**
 * @param boundaryLocation Topleft location of the boundary
 * @param boundaryScale Scale of the boundary from the Topleft location.
 * @return
 */
inline bool IsMouseInsideBoundary(const glm::vec2& mousePos, const glm::vec2& boundaryLocation, const glm::vec2& boundaryScale)
{
    return mousePos.x > boundaryLocation.x
        && mousePos.x < boundaryLocation.x + boundaryScale.x
        && mousePos.y > boundaryLocation.y
        && mousePos.y < boundaryLocation.y + boundaryScale.y;
}
