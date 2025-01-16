#include "input/steam/steam_action_manager.hpp"
#include "hashmap_utils.hpp"
#include "imgui.h"
#include "input/steam/steam_input_device_manager.hpp"
#include "log.hpp"
#include <filesystem>

SteamActionManager::SteamActionManager(const SteamInputDeviceManager& steamInputDeviceManager)
    : ActionManager(steamInputDeviceManager)
    , _steamInputDeviceManager(steamInputDeviceManager)
{
    // Use the following lines of code to override and test new input action that is not yet on steam servers
    // std::string actionManifestFilePath = std::filesystem::current_path().string();
    // actionManifestFilePath.append("\\assets\\input\\steam_input_manifest.vdf");
    // SteamInput()->SetInputActionManifestFilePath(actionManifestFilePath.c_str());
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

bool SteamActionManager::CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button, DigitalActionType inputType) const
{
    // A bit of a waist honestly, as this gets called every time when an action has a digital gamepad input to check,
    // but Steam checks all the gamepad inputs for an action for us. I didn't really find a clean way to do it another way for now.
    // So we will just take the hit whenever we have multiple gamepad digital inputs bound to 1 action and check multiple times even when not needed.

    switch (inputType)
    {
    case DigitalActionType::ePressed:
    {
        bool current = UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
        bool previous = UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);
        return current && !previous;
    }

    case DigitalActionType::eReleased:
    {
        bool current = UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
        bool previous = UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);
        return !current && previous;
    }

    case DigitalActionType::eHold:
    {
        return UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
    }
    }

    return false;
}

glm::vec2 SteamActionManager::CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadAnalog gamepadAnalog) const
{
    if (!_inputDeviceManager.IsGamepadAvailable())
    {
        return { 0.0f, 0.0f };
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadAnalogActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadAnalogActionsCache.end())
    {
        bblog::error("[Input] Failed to find analog action cache \"{}\" in the current active action set \"{}\"", actionName, _gameActions[_activeActionSet].name);
        return { 0.0f, 0.0f };
    }

    ControllerAnalogActionData_t analogActionData = SteamInput()->GetAnalogActionData(_steamInputDeviceManager.GetGamepadHandle(), itr->second);
    return { _inputDeviceManager.ClampGamepadAxisDeadzone(analogActionData.x), _inputDeviceManager.ClampGamepadAxisDeadzone(analogActionData.y) };
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