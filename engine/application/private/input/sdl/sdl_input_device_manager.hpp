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

    [[nodiscard]] bool IsGamepadButtonPressed(GamepadButton button) const;
    [[nodiscard]] bool IsGamepadButtonHeld(GamepadButton button) const;
    [[nodiscard]] bool IsGamepadButtonReleased(GamepadButton button) const;

    // Returns the given axis input from -1 to 1
    [[nodiscard]] float GetGamepadAxis(GamepadAxis axis) const;

    // Returns the given axis input from -1 to 1 with no deadzone handling
    [[nodiscard]] float GetRawGamepadAxis(GamepadAxis axis) const;

    // Returns whether a controller is connected and can be used for input
    [[nodiscard]] virtual bool IsGamepadAvailable() const final { return _gamepad.sdlHandle != nullptr; }

private:
    struct Gamepad : InputDevice<GamepadButton>
    {
        SDL_Gamepad* sdlHandle {};
    } _gamepad {};

    void CloseGamepad();
};