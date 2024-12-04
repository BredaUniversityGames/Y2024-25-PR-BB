#pragma once

#include "input_codes/gamepad.hpp"
#include "input_codes/keys.hpp"
#include "input_codes/mousebuttons.hpp"
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

    [[nodiscard]] bool IsKeyPressed(KeyboardCode key) const;
    [[nodiscard]] bool IsKeyHeld(KeyboardCode key) const;
    [[nodiscard]] bool IsKeyReleased(KeyboardCode key) const;

    [[nodiscard]] bool IsMouseButtonPressed(MouseButton button) const;
    [[nodiscard]] bool IsMouseButtonHeld(MouseButton button) const;
    [[nodiscard]] bool IsMouseButtonReleased(MouseButton button) const;
    void GetMousePosition(int& x, int& y) const;

    [[nodiscard]] bool IsGamepadButtonPressed(GamepadButton button) const;
    [[nodiscard]] bool IsGamepadButtonHeld(GamepadButton button) const;
    [[nodiscard]] bool IsGamepadButtonReleased(GamepadButton button) const;

    // Returns the given axis input from -1 to 1
    [[nodiscard]] float GetGamepadAxis(GamepadAxis axis) const;

    // Returns whether a controller is connected and can be used for input
    [[nodiscard]] bool IsGamepadAvailable() const { return _gamepad.sdlHandle != nullptr; }

private:
    static constexpr float MIN_GAMEPAD_AXIS = -1.0f;
    static constexpr float MAX_GAMEPAD_AXIS = 1.0f;
    static constexpr float INNER_GAMEPAD_DEADZONE = 0.1f;
    static constexpr float OUTER_GAMEPAD_DEADZONE = 0.95f;

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

    float ClampDeadzone(float input, float innerDeadzone, float outerDeadzone) const;
    void CloseGamepad();
};