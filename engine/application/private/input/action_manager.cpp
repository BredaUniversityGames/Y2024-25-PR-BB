#include "input/action_manager.hpp"
#include "input/input_manager.hpp"

ActionManager::ActionManager(const InputManager& inputManager)
    : _inputManager(inputManager)
{
}

void ActionManager::Update()
{

}

void ActionManager::SetGameActions(const GameActions& gameActions)
{
    _gameActions = gameActions;
}

void ActionManager::SetActiveActionSet(std::string_view actionSetName)
{
    auto itr = std::find_if(
    _gameActions.begin(), _gameActions.end(),
    [actionSetName](const ActionSet& actionSet) { return actionSet.name == actionSetName;});

    if (itr == _gameActions.end())
    {
        bblog::error("[Input] Failed to find action set: \"{}\"", actionSetName);
        return;
    }

    uint32_t index = itr - _gameActions.begin();
    _activeActionSet = index;
}

bool ActionManager::GetDigitalAction(std::string_view actionName) const
{
    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& digitalActions = actionSet.digitalActions;

    auto itr = std::find_if(digitalActions.begin(), digitalActions.end(),
        [actionName](const DigitalAction& action) { return action.name == actionName;});
    if (itr == actionSet.digitalActions.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return false;
    }

    return CheckDigitalInput(*itr);
}

void ActionManager::GetAnalogAction(std::string_view actionName, float& x, float& y) const
{
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

bool ActionManager::CheckDigitalInput(const DigitalAction &action) const
{
    for (const DigitalInputAction& input : action.inputs)
    {
        bool result = std::visit([&](auto& arg)
            {
                return CheckInput(arg, input.type);
            }, input.code);

        if (result)
        {
            return true;
        }
    }

    return false;
}

void ActionManager::CheckAnalogInput(const AnalogAction& action, float& x, float& y) const
{
    if (!_inputManager.IsGamepadAvailable())
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

        x = _inputManager.GetGamepadAxis(axes.first);
        y = _inputManager.GetGamepadAxis(axes.second);

        // First actions with input have priority, so if any input is found return
        if (x != 0.0f || y != 0.0f)
        {
            return;
        }
    }
}

bool ActionManager::CheckInput(KeyboardCode code, DigitalInputActionType inputType) const
{
    switch (inputType)
    {
        case DigitalInputActionType::Pressed:
        {
            return _inputManager.IsKeyPressed(code);
        }

        case DigitalInputActionType::Released:
        {
            return _inputManager.IsKeyReleased(code);
        }

        case DigitalInputActionType::Hold:
        {
            return _inputManager.IsKeyHeld(code);
        }
    }

    return false;
}

bool ActionManager::CheckInput(MouseButton button, DigitalInputActionType inputType) const
{
    switch (inputType)
    {
        case DigitalInputActionType::Pressed:
        {
            return _inputManager.IsMouseButtonPressed(button);
        }

        case DigitalInputActionType::Released:
        {
            return _inputManager.IsMouseButtonReleased(button);
        }

        case DigitalInputActionType::Hold:
        {
            return _inputManager.IsMouseButtonHeld(button);
        }
    }

    return false;
}

bool ActionManager::CheckInput(GamepadButton button, DigitalInputActionType inputType) const
{
    switch (inputType)
    {
        case DigitalInputActionType::Pressed:
        {
            return _inputManager.IsGamepadButtonPressed(button);
        }

        case DigitalInputActionType::Released:
        {
            return _inputManager.IsGamepadButtonReleased(button);
        }

        case DigitalInputActionType::Hold:
        {
            return _inputManager.IsGamepadButtonHeld(button);
        }
    }

    return false;
}