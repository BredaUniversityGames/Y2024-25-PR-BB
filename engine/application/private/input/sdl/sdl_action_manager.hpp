#pragma once
#include "input/action_manager.hpp"

class SDLInputDeviceManager;

// Input action manager for SDL devices.
class SDLActionManager final : public ActionManager
{
public:
    SDLActionManager(const SDLInputDeviceManager& sdlInputDeviceManager);

private:
    const SDLInputDeviceManager& _sdlInputDeviceManager;

    NO_DISCARD DigitalActionType CheckInput(std::string_view actionName, GamepadButton button) const final;
    NO_DISCARD glm::vec2 CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadAnalog gamepadAnalog) const final;
};
