#include "input/action_manager.hpp"
#include "input/input_manager.hpp"

ActionManager::ActionManager(const InputManager& inputManager)
    : _inputManager(inputManager)
{
    GameActions gameActions{};

    ActionSet& actionSet = gameActions.emplace_back();
    actionSet.name = "FlyCamera";

    DigitalAction exitAction{};
    exitAction.name = "Exit";
    exitAction.inputs.emplace_back(DigitalInputActionType::Pressed, KeyboardCode::eY);
    exitAction.inputs.emplace_back(DigitalInputActionType::Released, MouseButton::eBUTTON_RIGHT);
    exitAction.inputs.emplace_back(DigitalInputActionType::Hold, GamepadButton::eGAMEPAD_BUTTON_NORTH);
    exitAction.inputs.emplace_back(DigitalInputActionType::Hold, KeyboardCode::eZ);

    AnalogAction moveAction{};
    moveAction.name = "Move";

    AnalogAction cameraAction{};
    cameraAction.name = "Camera";

    actionSet.digitalActions.push_back(exitAction);
    actionSet.analogActions.push_back(moveAction);
    actionSet.analogActions.push_back(cameraAction);

    SetGameActions(gameActions);
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

void ActionManager::GetAnalogAction(std::string_view, float& x, float& y) const
{
    x = 1;
    y = 1;
}

bool ActionManager::CheckDigitalInput(const DigitalAction &action) const
{
    for (const DigitalInputAction& input : action.inputs)
    {
        if (std::holds_alternative<KeyboardCode>(input.code))
        {
            if (CheckKeyboardInput(std::get<KeyboardCode>(input.code), input.type))
            {
                return true;
            }
        }
        else if (std::holds_alternative<GamepadButton>(input.code))
        {
            if (CheckGamepadInput(std::get<GamepadButton>(input.code), input.type))
            {
                return true;
            }
        }
        else if (std::holds_alternative<MouseButton>(input.code))
        {
            if (CheckMouseInput(std::get<MouseButton>(input.code), input.type))
            {
                return true;
            }
        }
    }

    return false;
}

bool ActionManager::CheckKeyboardInput(KeyboardCode code, DigitalInputActionType inputType) const
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

bool ActionManager::CheckMouseInput(MouseButton button, DigitalInputActionType inputType) const
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

bool ActionManager::CheckGamepadInput(GamepadButton button, DigitalInputActionType inputType) const
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