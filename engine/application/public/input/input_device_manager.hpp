#pragma once
#include "input/input_codes/gamepad.hpp"
#include "input_codes/keys.hpp"
#include "input_codes/mousebuttons.hpp"
#include "common.hpp"
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

    NO_DISCARD bool IsKeyPressed(KeyboardCode key) const;
    NO_DISCARD bool IsKeyHeld(KeyboardCode key) const;
    NO_DISCARD bool IsKeyReleased(KeyboardCode key) const;

    NO_DISCARD bool IsMouseButtonPressed(MouseButton button) const;
    NO_DISCARD bool IsMouseButtonHeld(MouseButton button) const;
    NO_DISCARD bool IsMouseButtonReleased(MouseButton button) const;
    void GetMousePosition(int32_t& x, int32_t& y) const;

    NO_DISCARD virtual bool IsGamepadAvailable() const = 0;
    NO_DISCARD virtual GamepadType GetGamepadType() const = 0;
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
