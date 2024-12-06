#include "input/steam_action_manager.hpp"

SteamActionManager::SteamActionManager(const InputManager& inputManager)
  : ActionManager(inputManager)
{
}

void SteamActionManager::Update()
{
    ActionManager::Update();
    UpdateActiveController();

    if (!IsControllerAvailable() || _gameActions.empty())
    {
        return;
    }

    SteamInput()->ActivateActionSet(_inputHandle, _steamGameActionsCache[_activeActionSet].actionSetHandle);

    UpdateSteamControllerInputState();
}

void SteamActionManager::SetGameActions(const GameActions& gameActions)
{
    ActionManager::SetGameActions(gameActions);

    _currentControllerState.clear();
    _prevControllerState.clear();

    // Caching Steam Input API handles
    _steamGameActionsCache.resize(_gameActions.size());

    for (uint32_t i = 0; i < _gameActions.size(); ++i)
    {
        SteamActionSetCache& cache = _steamGameActionsCache[i];

        cache.actionSetHandle = SteamInput()->GetActionSetHandle(_gameActions[i].name.c_str());

        for (const DigitalAction& action : _gameActions[i].digitalActions)
        {
            cache.gamepadDigitalActionsCache.emplace(action.name, SteamInput()->GetDigitalActionHandle(action.name.c_str()));
        }

        for (const AnalogAction& action : _gameActions[i].analogActions)
        {
            cache.gamepadAnalogActionsCache.emplace(action.name, SteamInput()->GetAnalogActionHandle(action.name.c_str()));
        }
    }
}

void SteamActionManager::GetAnalogAction(std::string_view actionName, float &x, float &y) const
{
    if (!IsControllerAvailable() || _gameActions.empty())
    {
        return;
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadAnalogActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadAnalogActionsCache.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return;
    }

    ControllerAnalogActionData_t analogActionData = SteamInput()->GetAnalogActionData(_inputHandle, itr->second);

    x = analogActionData.x;
    y = analogActionData.y;
}

bool SteamActionManager::CheckDigitalInput(const DigitalAction& action) const
{
    for (const DigitalInputAction& input : action.inputs)
    {
        bool result = std::visit([&](auto& arg)
            {
                return CheckInput(action.name, arg, action.type);
            }, input);

        if (result)
        {
            return true;
        }
    }

    return false;
}

bool SteamActionManager::CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button, DigitalActionType inputType) const
{
    switch (inputType)
    {
        case DigitalActionType::Pressed:
        {
            bool current = detail::UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
            bool previous = detail::UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);
            return current && !previous;
        }

        case DigitalActionType::Released:
        {
            bool current = detail::UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
            bool previous = detail::UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);
            return !current && previous;
        }

        case DigitalActionType::Hold:
        {
            return detail::UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
        }
    }

    return false;
}

bool SteamActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, KeyboardCode code, DigitalActionType inputType) const
{
    return ActionManager::CheckInput(code, inputType);
}

bool SteamActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, MouseButton button, DigitalActionType inputType) const
{
    return ActionManager::CheckInput(button, inputType);
}

void SteamActionManager::UpdateActiveController()
{
    std::array<InputHandle_t, STEAM_CONTROLLER_MAX_COUNT> handles {};
    int numActive = SteamInput()->GetConnectedControllers(handles.data());

    // If there's an active controller, and if we're not already using it, select the first one.
    if (numActive && (_inputHandle != handles[0]))
    {
        _inputHandle = handles[0];
    }
}

void SteamActionManager::UpdateSteamControllerInputState()
{
    _prevControllerState = _currentControllerState;
    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    for (const auto& [actionName, actionHandle] : actionSetCache.gamepadDigitalActionsCache)
    {
        ControllerDigitalActionData_t digitalData = SteamInput()->GetDigitalActionData(_inputHandle, actionHandle);
        _currentControllerState[actionName] = digitalData.bState;
    }
}