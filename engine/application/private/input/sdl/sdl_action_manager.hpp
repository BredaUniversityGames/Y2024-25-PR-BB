#pragma once
#include "input/action_manager.hpp"

class SDLInputDeviceManager;

// Input action manager for SDL devices.
class SDLActionManager final : public ActionManager
{
public:
    SDLActionManager(const SDLInputDeviceManager& sdlInputDeviceManager);

    // Axis actions.
    virtual glm::vec2 GetAnalogAction(std::string_view actionName) const final;

private:
    const SDLInputDeviceManager& _sdlInputDeviceManager;

    glm::vec2 CheckAnalogInput(const AnalogAction& action) const;
    [[nodiscard]] virtual bool CheckInput(std::string_view actionName, GamepadButton button, DigitalActionType inputType) const final;
};