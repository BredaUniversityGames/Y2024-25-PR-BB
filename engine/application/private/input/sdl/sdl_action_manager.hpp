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

    NO_DISCARD virtual DigitalActionType CheckInput(std::string_view actionName, GamepadButton button) const final;
    NO_DISCARD virtual glm::vec2 CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadAnalog gamepadAnalog) const final;

    NO_DISCARD virtual std::vector<std::string> GetDigitalActionControllerGlyphImagePaths(std::string_view actionName) const final;
    NO_DISCARD virtual std::vector<std::string> GetAnalogActionControllerGlyphImagePaths(std::string_view actionName) const final;
};
