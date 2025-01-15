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

    [[nodiscard]] virtual bool CheckInput(std::string_view actionName, GamepadButton button, DigitalActionType inputType) const final;
    [[nodiscard]] virtual glm::vec2 CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadAnalog gamepadAnalog) const final;
};