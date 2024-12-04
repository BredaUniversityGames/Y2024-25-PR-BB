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
    void GetMousePosition(int32_t& x, int32_t& y) const;

    [[nodiscard]] bool IsGamepadButtonPressed(GamepadButton button) const;
    [[nodiscard]] bool IsGamepadButtonHeld(GamepadButton button) const;
    [[nodiscard]] bool IsGamepadButtonReleased(GamepadButton button) const;

    // Returns the given axis input from -1 to 1
    [[nodiscard]] float GetGamepadAxis(GamepadAxis axis) const;

    // Returns the given axis input from -1 to 1 with no deadzone handling
    [[nodiscard]] float GetRawGamepadAxis(GamepadAxis axis) const;

    // Returns whether a controller is connected and can be used for input
    [[nodiscard]] bool IsGamepadAvailable() const { return _gamepad.sdlHandle != nullptr; }

private:
    static constexpr float MIN_GAMEPAD_AXIS = -1.0f;
    static constexpr float MAX_GAMEPAD_AXIS = 1.0f;
    static constexpr float INNER_GAMEPAD_DEADZONE = 0.1f;
    static constexpr float OUTER_GAMEPAD_DEADZONE = 0.95f;

    template <typename T>
    struct InputDevice
    {
        std::unordered_map<T, bool> inputPressed {};
        std::unordered_map<T, bool> inputHeld {};
        std::unordered_map<T, bool> inputReleased {};
    };

    struct Mouse : InputDevice<MouseButton>
    {
        float positionX {}, positionY {};
    } _mouse {};

    InputDevice<KeyboardCode> _keyboard {};

    struct Gamepad : InputDevice<GamepadButton>
    {
        SDL_Gamepad* sdlHandle {};
    } _gamepad {};

    float ClampDeadzone(float input, float innerDeadzone, float outerDeadzone) const;
    void CloseGamepad();
};