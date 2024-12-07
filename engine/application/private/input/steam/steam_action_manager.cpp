#include "input/steam/steam_action_manager.hpp"
#include "input/steam/steam_input_device_manager.hpp"
#include "hashmap_utils.hpp"

SteamActionManager::SteamActionManager(const SteamInputDeviceManager& steamInputDeviceManager)
    : ActionManager(steamInputDeviceManager)
    , _steamInputDeviceManager(steamInputDeviceManager)
{
}

void SteamActionManager::Update()
{
    ActionManager::Update();

    if (!_inputDeviceManager.IsGamepadAvailable() || _gameActions.empty())
    {
        return;
    }

    SteamInput()->ActivateActionSet(_steamInputDeviceManager.GetGamepadHandle(), _steamGameActionsCache[_activeActionSet].actionSetHandle);

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
    if (!_inputDeviceManager.IsGamepadAvailable() || _gameActions.empty())
    {
        return;
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadAnalogActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadAnalogActionsCache.end())
    {
        bblog::error("[Input] Failed to find analog action \"{}\" in the current active action set \"{}\"", actionName, _gameActions[_activeActionSet].name);
        return;
    }

    ControllerAnalogActionData_t analogActionData = SteamInput()->GetAnalogActionData(_steamInputDeviceManager.GetGamepadHandle(), itr->second);

    x = _inputDeviceManager.ClampGamepadAxisDeadzone(analogActionData.x);
    y = _inputDeviceManager.ClampGamepadAxisDeadzone(analogActionData.y);
}

bool SteamActionManager::CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button, DigitalActionType inputType) const
{
    // A bit of a waist honestly, as this gets called every time when an action has a digital gamepad input to check,
    // but Steam checks all the gamepad inputs for an action for us. I didn't really find a clean way to do it another way for now.
    // So we will just take the hit whenever we have multiple gamepad digital inputs bound to 1 action and check multiple times even when not needed.

    switch (inputType)
    {
        case DigitalActionType::Pressed:
        {
            bool current = UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
            bool previous = UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);
            return current && !previous;
        }

        case DigitalActionType::Released:
        {
            bool current = UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
            bool previous = UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);
            return !current && previous;
        }

        case DigitalActionType::Hold:
        {
            return UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
        }
    }

    return false;
}

void SteamActionManager::UpdateSteamControllerInputState()
{
    _prevControllerState = _currentControllerState;
    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    for (const auto& [actionName, actionHandle] : actionSetCache.gamepadDigitalActionsCache)
    {
        ControllerDigitalActionData_t digitalData = SteamInput()->GetDigitalActionData(_steamInputDeviceManager.GetGamepadHandle(), actionHandle);
        _currentControllerState[actionName] = digitalData.bState;
    }
}