#include "input/steam/steam_action_manager.hpp"
#include "hashmap_utils.hpp"
#include "input/steam/steam_input_device_manager.hpp"
#include "log.hpp"
#include <array>
#include <filesystem>

SteamActionManager::SteamActionManager(const SteamInputDeviceManager& steamInputDeviceManager)
    : ActionManager(steamInputDeviceManager)
    , _steamInputDeviceManager(steamInputDeviceManager)
{
    // Use the following lines of code to override and test new input action that is not yet on steam servers
    // std::string actionManifestFilePath = std::filesystem::current_path().string();
    // actionManifestFilePath.append("\\content\\input\\steam_input_manifest.vdf");
    // SteamInput()->SetInputActionManifestFilePath(actionManifestFilePath.c_str());
}

void SteamActionManager::Update()
{
    ActionManager::Update();

    if (!_inputDeviceManager.IsGamepadAvailable() || _gameActions.empty())
    {
        return;
    }

    // Make sure we have the steam handles cached (could not be cached in case of the controller being connected after the game startup)
    if (_steamGameActionsCache.empty())
    {
        CacheSteamInputHandles();
    }

    SteamInput()->ActivateActionSet(_steamInputDeviceManager.GetGamepadHandle(), _steamGameActionsCache[_activeActionSet].actionSetHandle);
    UpdateSteamControllerInputState();
}

void SteamActionManager::SetGameActions(const GameActions& gameActions)
{
    ActionManager::SetGameActions(gameActions);

    _currentControllerState.clear();
    _prevControllerState.clear();

    CacheSteamInputHandles();

    if (!_gameActions.empty())
    {
        SetActiveActionSet(_gameActions[0].name);
    }
}

DigitalActionType SteamActionManager::CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button) const
{
    DigitalActionType result {};

    bool current = UnorderedMapGetOr(_currentControllerState, { actionName.begin(), actionName.end() }, false);
    bool previous = UnorderedMapGetOr(_prevControllerState, { actionName.begin(), actionName.end() }, false);

    if (current && !previous)
    {
        result = result | DigitalActionType::ePressed;
    }
    if (current)
    {
        result = result | DigitalActionType::eHeld;
    }
    if (!current && previous)
    {
        result = result | DigitalActionType::eReleased;
    }

    return result;
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

std::vector<BindingOriginVisual> SteamActionManager::GetDigitalActionGamepadOriginVisual(const DigitalAction& action) const
{
    if (!_steamInputDeviceManager.IsGamepadAvailable())
    {
        return {};
    }

    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", action.name);
        return {};
    }

    std::vector<BindingOriginVisual> visuals = ActionManager::GetDigitalActionGamepadOriginVisual(action);

    // We found custom visuals, so we use those
    if (!visuals.empty())
    {
        return visuals;
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadDigitalActionsCache.find(action.name.data());
    if (itr == actionSetCache.gamepadDigitalActionsCache.end())
    {
        bblog::error("[Input] Failed to find digital action cache \"{}\" in the current active action set \"{}\"", action.name, _gameActions[_activeActionSet].name);
        return {};
    }

    std::array<EInputActionOrigin, STEAM_INPUT_MAX_ORIGINS> origins {};
    uint32_t originsNum = SteamInput()->GetDigitalActionOrigins(_steamInputDeviceManager.GetGamepadHandle(), actionSetCache.actionSetHandle, itr->second, origins.data());

    // Action not bound to any input
    if (originsNum == 0)
    {
        return {};
    }

    for (uint32_t i = 0; i < originsNum; ++i)
    {
        BindingOriginVisual& visual = visuals.emplace_back();
        visual.bindingInputName = SteamInput()->GetStringForActionOrigin(origins[i]);
        visual.glyphImagePath = SteamInput()->GetGlyphPNGForActionOrigin(origins[i], k_ESteamInputGlyphSize_Large, 0);
    }

    return visuals;
}

std::vector<BindingOriginVisual> SteamActionManager::GetAnalogActionGamepadOriginVisual(const AnalogAction& action) const
{
    if (!_steamInputDeviceManager.IsGamepadAvailable())
    {
        return {};
    }

    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", action.name);
        return {};
    }

    std::vector<BindingOriginVisual> visuals = ActionManager::GetAnalogActionGamepadOriginVisual(action);

    // We found custom visuals, so we use those
    if (!visuals.empty())
    {
        return visuals;
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadAnalogActionsCache.find(action.name.data());
    if (itr == actionSetCache.gamepadAnalogActionsCache.end())
    {
        bblog::error("[Input] Failed to find analog action cache \"{}\" in the current active action set \"{}\"", action.name, _gameActions[_activeActionSet].name);
        return {};
    }

    std::array<EInputActionOrigin, STEAM_INPUT_MAX_ORIGINS> origins {};
    uint32_t originsNum = SteamInput()->GetAnalogActionOrigins(_steamInputDeviceManager.GetGamepadHandle(), actionSetCache.actionSetHandle, itr->second, origins.data());

    // Action not bound to any input
    if (originsNum == 0)
    {
        return {};
    }

    for (uint32_t i = 0; i < originsNum; ++i)
    {
        BindingOriginVisual& visual = visuals.emplace_back();
        visual.bindingInputName = SteamInput()->GetStringForActionOrigin(origins[i]);
        visual.glyphImagePath = SteamInput()->GetGlyphPNGForActionOrigin(origins[i], k_ESteamInputGlyphSize_Large, 0);
    }

    return visuals;
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

void SteamActionManager::CacheSteamInputHandles()
{
    if (!_inputDeviceManager.IsGamepadAvailable())
    {
        return;
    }

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
