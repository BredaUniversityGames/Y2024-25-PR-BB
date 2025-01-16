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

DigitalActionResult ActionManager::GetDigitalAction(std::string_view actionName) const
{
    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return DigitalActionResult {};
    }

    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& digitalActions = actionSet.digitalActions;

    auto itr = std::find_if(digitalActions.begin(), digitalActions.end(),
        [actionName](const DigitalAction& action)
        { return action.name == actionName; });
    if (itr == actionSet.digitalActions.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return DigitalActionResult {};
    }

    return DigitalActionResult { CheckDigitalInput(*itr) };
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

DigitalActionType ActionManager::CheckDigitalInput(const DigitalAction& action) const
{
    DigitalActionType result {};

    for (const DigitalInputBinding& input : action.inputs)
    {
        result = result | std::visit([&](auto& arg)
                     { return CheckInput(action.name, arg); }, input);
    }

    return result;
}

DigitalActionType ActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, KeyboardCode code) const
{
    DigitalActionType result {};

    if (_inputDeviceManager.IsKeyPressed(code))
    {
        result = result | DigitalActionType::ePressed;
    }
    if (_inputDeviceManager.IsKeyHeld(code))
    {
        result = result | DigitalActionType::eHeld;
    }
    if (_inputDeviceManager.IsKeyReleased(code))
    {
        result = result | DigitalActionType::eReleased;
    }

    return result;
}

DigitalActionType ActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, MouseButton button) const
{
    DigitalActionType result {};

    if (_inputDeviceManager.IsMouseButtonPressed(button))
    {
        result = result | DigitalActionType::ePressed;
    }
    if (_inputDeviceManager.IsMouseButtonHeld(button))
    {
        result = result | DigitalActionType::eHeld;
    }
    if (_inputDeviceManager.IsMouseButtonReleased(button))
    {
        result = result | DigitalActionType::eReleased;
    }

    return result;
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
