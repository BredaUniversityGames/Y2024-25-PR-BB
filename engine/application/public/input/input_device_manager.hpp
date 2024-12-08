#pragma once
#include "input/input_codes/gamepad.hpp"
#include "input_codes/keys.hpp"
#include "input_codes/mousebuttons.hpp"
#include <unordered_map>

union SDL_Event;

// Abstract class which manages SDL mouse and keyboard input devices.
// Inherit to manage gamepad devices.
class InputDeviceManager
{
public:
    InputDeviceManager();
    virtual ~InputDeviceManager() = default;

    virtual void Update();
    virtual void UpdateEvent(const SDL_Event& event);

    [[nodiscard]] bool IsKeyPressed(KeyboardCode key) const;
    [[nodiscard]] bool IsKeyHeld(KeyboardCode key) const;
    [[nodiscard]] bool IsKeyReleased(KeyboardCode key) const;

    [[nodiscard]] bool IsMouseButtonPressed(MouseButton button) const;
    [[nodiscard]] bool IsMouseButtonHeld(MouseButton button) const;
    [[nodiscard]] bool IsMouseButtonReleased(MouseButton button) const;
    void GetMousePosition(int32_t& x, int32_t& y) const;

    [[nodiscard]] virtual bool IsGamepadAvailable() const = 0;
    float ClampGamepadAxisDeadzone(float input) const;

protected:
    template <typename T>
    struct InputDevice
    {
        std::unordered_map<T, bool> inputPressed {};
        std::unordered_map<T, bool> inputHeld {};
        std::unordered_map<T, bool> inputReleased {};
    };

private:
    static constexpr float MIN_GAMEPAD_AXIS = -1.0f;
    static constexpr float MAX_GAMEPAD_AXIS = 1.0f;
    static constexpr float INNER_GAMEPAD_DEADZONE = 0.1f;
    static constexpr float OUTER_GAMEPAD_DEADZONE = 0.95f;

    struct Mouse : InputDevice<MouseButton>
    {
        float positionX {}, positionY {};
    } _mouse {};

    InputDevice<KeyboardCode> _keyboard {};
};