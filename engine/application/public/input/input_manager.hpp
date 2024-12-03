#pragma once

#include "input_codes/gamepad.hpp"
#include "input_codes/keys.hpp"
#include "input_codes/mousebuttons.hpp"
#include <SDL3/SDL_joystick.h>
#include <unordered_map>

union SDL_Event;
struct SDL_Gamepad;

class InputManager
{
public:
    InputManager();
    ~InputManager();

    void Update();
    void UpdateEvent(const SDL_Event& event);

    bool IsKeyPressed(KeyboardCode key) const;
    bool IsKeyHeld(KeyboardCode key) const;
    bool IsKeyReleased(KeyboardCode key) const;

    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonHeld(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    void GetMousePosition(int& x, int& y) const;

    bool IsGamepadButtonPressed(GamepadButton button) const;
    bool IsGamepadButtonHeld(GamepadButton button) const;
    bool IsGamepadButtonReleased(GamepadButton button) const;

    // Returns the given axis input from -1 to 1
    float GetGamepadAxis(GamepadAxis axis) const;

    // Returns whether a controller is connected and can be used for input
    bool IsControllerAvailable() const { return _gamepad.sdlHandle != nullptr; }

private:
    struct Mouse
    {
        std::unordered_map<MouseButton, bool> buttonPressed {};
        std::unordered_map<MouseButton, bool> buttonHeld {};
        std::unordered_map<MouseButton, bool> buttonReleased {};
        float positionX {}, positionY {};
    } _mouse {};

    struct Keyboard
    {
        std::unordered_map<KeyboardCode, bool> keyPressed {};
        std::unordered_map<KeyboardCode, bool> keyHeld {};
        std::unordered_map<KeyboardCode, bool> keyReleased {};
    } _keyboard;

    struct Gamepad
    {
        std::unordered_map<GamepadButton, bool> buttonPressed {};
        std::unordered_map<GamepadButton, bool> buttonHeld {};
        std::unordered_map<GamepadButton, bool> buttonReleased {};
        SDL_Gamepad* sdlHandle {};
    } _gamepad {};
};