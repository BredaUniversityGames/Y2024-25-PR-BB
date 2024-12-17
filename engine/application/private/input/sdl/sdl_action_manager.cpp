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

glm::vec2 SDLActionManager::GetAnalogAction(std::string_view actionName) const
{
    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return { 0.0f, 0.0f };
    }

    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& analogActions = actionSet.analogActions;

    auto itr = std::find_if(analogActions.begin(), analogActions.end(),
        [actionName](const AnalogAction& action)
        { return action.name == actionName; });
    if (itr == actionSet.analogActions.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return { 0.0f, 0.0f };
    }

    return CheckAnalogInput(*itr);
}

glm::vec2 SDLActionManager::CheckAnalogInput(const AnalogAction& action) const
{
    glm::vec2 result = { 0.0f, 0.0f };

    if (!_inputDeviceManager.IsGamepadAvailable())
    {
        return result;
    }

    for (const AnalogInputBinding& input : action.inputs)
    {
        switch (input)
        {
        // Steam Input allows to use DPAD as analog input, so we do the same for SDL input
        case AnalogInputBinding::eDPAD:
        {
            result.x = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_LEFT)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_RIGHT));
            result.y = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_DOWN)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_UP));
            break;
        }

        case AnalogInputBinding::eAXIS_LEFT:
        case AnalogInputBinding::eAXIS_RIGHT:
        {
            static const std::unordered_map<GamepadAnalog, std::pair<GamepadAxis, GamepadAxis>> GAMEPAD_ANALOG_AXIS_MAPPING {
                { GamepadAnalog::eAXIS_LEFT, { GamepadAxis::eLEFTX, GamepadAxis::eLEFTY } },
                { GamepadAnalog::eAXIS_RIGHT, { GamepadAxis::eRIGHTX, GamepadAxis::eRIGHTY } },
            };

            const std::pair<GamepadAxis, GamepadAxis> axes = GAMEPAD_ANALOG_AXIS_MAPPING.at(input);
            result.x = _sdlInputDeviceManager.GetGamepadAxis(axes.first);
            result.y = _sdlInputDeviceManager.GetGamepadAxis(axes.second);
            break;
        }

        default:
        {
            bblog::error("[Input] Unsupported analog input \"{}\" for action: \"{}\"", magic_enum::enum_name(input), action.name);
            break;
        }
        }

        // First actions with input have priority, so if any input is found return
        if (result.x != 0.0f || result.y != 0.0f)
        {
            return result;
        }
    }

    return result;
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