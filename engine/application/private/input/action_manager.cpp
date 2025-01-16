#include "input/action_manager.hpp"
#include "log.hpp"
#include <algorithm>

ActionManager::ActionManager(const InputDeviceManager& inputDeviceManager)
    : _inputDeviceManager(inputDeviceManager)
{
}

void ActionManager::SetActiveActionSet(std::string_view actionSetName)
{
    auto itr = std::find_if(_gameActions.begin(), _gameActions.end(),
        [actionSetName](const ActionSet& actionSet)
        { return actionSet.name == actionSetName; });

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
    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return false;
    }

    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& digitalActions = actionSet.digitalActions;

    auto itr = std::find_if(digitalActions.begin(), digitalActions.end(),
        [actionName](const DigitalAction& action)
        { return action.name == actionName; });
    if (itr == actionSet.digitalActions.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return false;
    }

    return CheckDigitalInput(*itr);
}

glm::vec2 ActionManager::GetAnalogAction(std::string_view actionName) const
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

bool ActionManager::CheckDigitalInput(const DigitalAction& action) const
{
    for (const DigitalInputBinding& input : action.inputs)
    {
        bool result = std::visit([&](auto& arg)
            { return CheckInput(action.name, arg, action.type); }, input);

        if (result)
        {
            return true;
        }
    }

    return false;
}

bool ActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, KeyboardCode code, DigitalActionType inputType) const
{
    switch (inputType)
    {
    case DigitalActionType::ePressed:
    {
        return _inputDeviceManager.IsKeyPressed(code);
    }

    case DigitalActionType::eReleased:
    {
        return _inputDeviceManager.IsKeyReleased(code);
    }

    case DigitalActionType::eHold:
    {
        return _inputDeviceManager.IsKeyHeld(code);
    }
    }

    return false;
}

bool ActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, MouseButton button, DigitalActionType inputType) const
{
    switch (inputType)
    {
    case DigitalActionType::ePressed:
    {
        return _inputDeviceManager.IsMouseButtonPressed(button);
    }

    case DigitalActionType::eReleased:
    {
        return _inputDeviceManager.IsMouseButtonReleased(button);
    }

    case DigitalActionType::eHold:
    {
        return _inputDeviceManager.IsMouseButtonHeld(button);
    }
    }

    return false;
}

glm::vec2 ActionManager::CheckAnalogInput(const AnalogAction& action) const
{
    for (const AnalogInputBinding& input : action.inputs)
    {
        glm::vec2 result = std::visit([&](auto& arg)
            { return CheckInput(action.name, arg); }, input);

        // Return first input that is non-zero
        if (result.x != 0.0f || result.y != 0.0f)
        {
            return result;
        }
    }

    return { 0.0f, 0.0f };
}

glm::vec2 ActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, const KeyboardAnalog& keyboardAnalog) const
{
    return { _inputDeviceManager.IsKeyHeld(keyboardAnalog.right) - _inputDeviceManager.IsKeyHeld(keyboardAnalog.left), _inputDeviceManager.IsKeyHeld(keyboardAnalog.up) - _inputDeviceManager.IsKeyHeld(keyboardAnalog.down) };
}
