#pragma once
#include "input/input_device_manager.hpp"
#include "steam_include.hpp"

// Manages SDL mouse and keyboard devices and Steam Input gamepads
class SteamInputDeviceManager final : public InputDeviceManager
{
public:
    virtual void Update() final;

    // Returns whether a controller is connected and can be used for input.
    [[nodiscard]] virtual bool IsGamepadAvailable() const final { return _inputHandle != 0; }
    [[nodiscard]] InputHandle_t GetGamepadHandle() const { return _inputHandle; }

private:
    InputHandle_t _inputHandle {};
};