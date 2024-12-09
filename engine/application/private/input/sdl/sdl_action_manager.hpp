#pragma once
#include "input/action_manager.hpp"

class SDLInputDeviceManager;

// Input action manager for SDL devices.
class SDLActionManager final : public ActionManager
{
public:
    SDLActionManager(const SDLInputDeviceManager& sdlInputDeviceManager);

    // Axis actions.
    virtual void GetAnalogAction(std::string_view actionName, float& x, float& y) const final;

private:
    const SDLInputDeviceManager& _sdlInputDeviceManager;

    void CheckAnalogInput(const AnalogAction& action, float& x, float& y) const;
    [[nodiscard]] virtual bool CheckInput(std::string_view actionName, GamepadButton button, DigitalActionType inputType) const final;
};