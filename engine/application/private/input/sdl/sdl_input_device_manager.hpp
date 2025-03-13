#pragma once
#include "input/input_device_manager.hpp"

struct SDL_Gamepad;

// Manages SDL mouse, keyboard and gamepad devices
class SDLInputDeviceManager final : public InputDeviceManager
{
public:
    SDLInputDeviceManager();
    virtual ~SDLInputDeviceManager() final;

    virtual void Update() final;
    virtual void UpdateEvent(const SDL_Event& event) final;

    NO_DISCARD bool IsGamepadButtonPressed(GamepadButton button) const;
    NO_DISCARD bool IsGamepadButtonHeld(GamepadButton button) const;
    NO_DISCARD bool IsGamepadButtonReleased(GamepadButton button) const;

    // Returns the given axis input from -1 to 1
    NO_DISCARD float GetGamepadAxis(GamepadAxis axis) const;

    // Returns the given axis input from -1 to 1 with no deadzone handling
    NO_DISCARD float GetRawGamepadAxis(GamepadAxis axis) const;

    // Returns whether a controller is connected and can be used for input
    NO_DISCARD virtual bool IsGamepadAvailable() const final { return _gamepad.sdlHandle != nullptr; }
    NO_DISCARD virtual GamepadType GetGamepadType() const final;

private:
    struct Gamepad : InputDevice<GamepadButton>
    {
        SDL_Gamepad* sdlHandle {};
        bool prevLeftTriggerState = false;
        bool prevRightTriggerState = false;
    } _gamepad {};

    void CloseGamepad();
    void UpdateGamepadTriggerButtons();
};
