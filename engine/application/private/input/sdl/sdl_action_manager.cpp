#include "input/sdl/sdl_action_manager.hpp"
#include "input/sdl/sdl_input_device_manager.hpp"
#include <magic_enum.hpp>

SDLActionManager::SDLActionManager(const SDLInputDeviceManager& sdlInputDeviceManager)
    : ActionManager(sdlInputDeviceManager)
    , _sdlInputDeviceManager(sdlInputDeviceManager)
{
}

void SDLActionManager::GetAnalogAction(std::string_view actionName, float& x, float& y) const
{
    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return;
    }

    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& analogActions = actionSet.analogActions;

    auto itr = std::find_if(analogActions.begin(), analogActions.end(),
        [actionName](const AnalogAction& action)
        { return action.name == actionName; });
    if (itr == actionSet.analogActions.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return;
    }

    CheckAnalogInput(*itr, x, y);
}

void SDLActionManager::CheckAnalogInput(const AnalogAction& action, float& x, float& y) const
{
    if (!_inputDeviceManager.IsGamepadAvailable())
    {
        return;
    }

    for (const AnalogInputAction& input : action.inputs)
    {
        switch (input)
        {
        // Steam Input allows to use DPAD as analog input, so we do the same for SDL input
        case AnalogInputAction::eDPAD:
        {
            x = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_LEFT)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_RIGHT));
            y = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_DOWN)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_UP));
            break;
        }

        case AnalogInputAction::eAXIS_LEFT:
        case AnalogInputAction::eAXIS_RIGHT:
        {
            static const std::unordered_map<GamepadAnalog, std::pair<GamepadAxis, GamepadAxis>> GAMEPAD_ANALOG_AXIS_MAPPING {
                { GamepadAnalog::eAXIS_LEFT, { GamepadAxis::eLEFTX, GamepadAxis::eLEFTY } },
                { GamepadAnalog::eAXIS_RIGHT, { GamepadAxis::eRIGHTX, GamepadAxis::eRIGHTY } },
            };

            const std::pair<GamepadAxis, GamepadAxis> axes = GAMEPAD_ANALOG_AXIS_MAPPING.at(input);
            x = _sdlInputDeviceManager.GetGamepadAxis(axes.first);
            y = _sdlInputDeviceManager.GetGamepadAxis(axes.second);
            break;
        }

        default:
        {
            bblog::error("[Input] Unsupported analog input \"{}\" for action: \"{}\"", magic_enum::enum_name(input), action.name);
            break;
        }
        }

        // First actions with input have priority, so if any input is found return
        if (x != 0.0f || y != 0.0f)
        {
            return;
        }
    }
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