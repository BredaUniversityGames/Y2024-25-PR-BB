#include "input/sdl/sdl_action_manager.hpp"
#include "input/sdl/sdl_input_device_manager.hpp"

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
        [actionName](const AnalogAction& action) { return action.name == actionName;});
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

    static const std::unordered_map<GamepadAnalog, std::pair<GamepadAxis, GamepadAxis>> GAMEPAD_ANALOG_AXIS_MAPPING
    {
        { GamepadAnalog::eGAMEPAD_AXIS_LEFT, { GamepadAxis::eGAMEPAD_AXIS_LEFTX, GamepadAxis::eGAMEPAD_AXIS_LEFTY } },
        { GamepadAnalog::eGAMEPAD_AXIS_RIGHT, { GamepadAxis::eGAMEPAD_AXIS_RIGHTX, GamepadAxis::eGAMEPAD_AXIS_RIGHTY} }
    };

    for (const AnalogInputAction& input : action.inputs)
    {
        const std::pair<GamepadAxis, GamepadAxis> axes = GAMEPAD_ANALOG_AXIS_MAPPING.at(input);

        x = _sdlInputDeviceManager.GetGamepadAxis(axes.first);
        y = _sdlInputDeviceManager.GetGamepadAxis(axes.second);

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
        case DigitalActionType::Pressed:
        {
            return _sdlInputDeviceManager.IsGamepadButtonPressed(button);
        }

        case DigitalActionType::Released:
        {
            return _sdlInputDeviceManager.IsGamepadButtonReleased(button);
        }

        case DigitalActionType::Hold:
        {
            return _sdlInputDeviceManager.IsGamepadButtonHeld(button);
        }
    }

    return false;
}