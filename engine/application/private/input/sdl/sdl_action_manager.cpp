#include "input/sdl/sdl_action_manager.hpp"
#include "input/sdl/sdl_input_device_manager.hpp"
#include "log.hpp"
#include <algorithm>
#include <magic_enum.hpp>

SDLActionManager::SDLActionManager(const SDLInputDeviceManager& sdlInputDeviceManager)
    : ActionManager(sdlInputDeviceManager)
    , _sdlInputDeviceManager(sdlInputDeviceManager)
{
}

bool SDLActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadButton button, DigitalActionType inputType) const
{
    switch (inputType)
    {
    case DigitalActionType::ePressed:
    {
        return _sdlInputDeviceManager.IsGamepadButtonPressed(button);
    }

    case DigitalActionType::eReleased:
    {
        return _sdlInputDeviceManager.IsGamepadButtonReleased(button);
    }

    case DigitalActionType::eHold:
    {
        return _sdlInputDeviceManager.IsGamepadButtonHeld(button);
    }
    }

    return false;
}

glm::vec2 SDLActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadAnalog gamepadAnalog) const
{
    glm::vec2 result = { 0.0f, 0.0f };

    if (!_inputDeviceManager.IsGamepadAvailable())
    {
        return result;
    }

    switch (gamepadAnalog)
    {
    // Steam Input allows to use DPAD as analog input, so we do the same for SDL input
    case GamepadAnalog::eDPAD:
    {
        result.x = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_LEFT)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_RIGHT));
        result.y = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_DOWN)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_UP));
        break;
    }

    case GamepadAnalog::eAXIS_LEFT:
    case GamepadAnalog::eAXIS_RIGHT:
    {
        static const std::unordered_map<GamepadAnalog, std::pair<GamepadAxis, GamepadAxis>> GAMEPAD_ANALOG_AXIS_MAPPING {
            { GamepadAnalog::eAXIS_LEFT, { GamepadAxis::eLEFTX, GamepadAxis::eLEFTY } },
            { GamepadAnalog::eAXIS_RIGHT, { GamepadAxis::eRIGHTX, GamepadAxis::eRIGHTY } },
        };

        const std::pair<GamepadAxis, GamepadAxis> axes = GAMEPAD_ANALOG_AXIS_MAPPING.at(gamepadAnalog);
        result.x = _sdlInputDeviceManager.GetGamepadAxis(axes.first);
        result.y = _sdlInputDeviceManager.GetGamepadAxis(axes.second);
        break;
    }

    default:
    {
        bblog::error("[Input] Unsupported analog input \"{}\" for action: \"{}\"", magic_enum::enum_name(gamepadAnalog), actionName);
        break;
    }
    }

    return result;
}