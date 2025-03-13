#pragma once
#include "input/input_device_manager.hpp"
#include "steam_include.hpp"

// Manages SDL mouse and keyboard devices and Steam Input gamepads
class SteamInputDeviceManager final : public InputDeviceManager
{
public:
    SteamInputDeviceManager();
    virtual void Update() final;

    // Returns whether a controller is connected and can be used for input.
    NO_DISCARD virtual bool IsGamepadAvailable() const final { return _inputHandle != 0; }
    NO_DISCARD virtual GamepadType GetGamepadType() const final;
    NO_DISCARD InputHandle_t GetGamepadHandle() const { return _inputHandle; }

private:
    void UpdateControllerConnectivity();

    InputHandle_t _inputHandle {};
};
