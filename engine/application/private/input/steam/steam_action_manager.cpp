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

    if (!_gameActions.empty())
    {
        SetActiveActionSet(_gameActions[0].name);
    }
}

void SteamActionManager::SetActiveActionSet(std::string_view actionSetName)
{
    ActionManager::SetActiveActionSet(actionSetName);

    SteamInput()->ActivateActionSet(_steamInputDeviceManager.GetGamepadHandle(), _steamGameActionsCache[_activeActionSet].actionSetHandle);
    SteamInput()->RunFrame(); // Make sure a set is immediately used
}

std::vector<std::string> SteamActionManager::GetDigitalActionGamepadGlyphImagePaths(std::string_view actionName) const
{
    if (!_steamInputDeviceManager.IsGamepadAvailable())
    {
        return {};
    }

    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return {};
    }

    std::vector<std::string> glyphPaths = ActionManager::GetDigitalActionGamepadGlyphImagePaths(actionName);

    // There is a chance we could only find 1 custom glyph for the input binding, but not for others bindings for the same action.
    // In that case we return anyway as we also don't want to have multiple glyphs from different styles.
    if (!glyphPaths.empty())
    {
        return glyphPaths;
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadDigitalActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadDigitalActionsCache.end())
    {
        bblog::error("[Input] Failed to find digital action cache \"{}\" in the current active action set \"{}\"", actionName, _gameActions[_activeActionSet].name);
        return {};
    }

    std::array<EInputActionOrigin, STEAM_INPUT_MAX_ORIGINS> origins {};
    uint32_t originsNum = SteamInput()->GetDigitalActionOrigins(_steamInputDeviceManager.GetGamepadHandle(), actionSetCache.actionSetHandle, itr->second, origins.data());

    if (originsNum == 0)
    {
        bblog::error("[Input] Digital action \"{}\" is not bound to any input, couldn't find any glyph path", actionName);
        return {};
    }

    for (uint32_t i = 0; i < originsNum; ++i)
    {
        glyphPaths.emplace_back(SteamInput()->GetGlyphPNGForActionOrigin(origins[i], k_ESteamInputGlyphSize_Large, 0));
    }

    return glyphPaths;
}

std::vector<std::string> SteamActionManager::GetAnalogActionGamepadGlyphImagePaths(std::string_view actionName) const
{
    if (!_steamInputDeviceManager.IsGamepadAvailable())
    {
        return {};
    }

    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return {};
    }

    std::vector<std::string> glyphPaths = ActionManager::GetAnalogActionGamepadGlyphImagePaths(actionName);

    // There is a chance we could only find 1 custom glyph for the input binding, but not for others bindings for the same action.
    // In that case we return anyway as we also don't want to have multiple glyphs from different styles.
    if (!glyphPaths.empty())
    {
        return glyphPaths;
    }

    const SteamActionSetCache& actionSetCache = _steamGameActionsCache[_activeActionSet];

    auto itr = actionSetCache.gamepadAnalogActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadAnalogActionsCache.end())
    {
        bblog::error("[Input] Failed to find analog action cache \"{}\" in the current active action set \"{}\"", actionName, _gameActions[_activeActionSet].name);
        return {};
    }

    std::array<EInputActionOrigin, STEAM_INPUT_MAX_ORIGINS> origins {};
    SteamInput()->GetAnalogActionOrigins(_steamInputDeviceManager.GetGamepadHandle(), actionSetCache.actionSetHandle, itr->second, origins.data());

    if (origins[0] == k_EInputActionOrigin_None)
    {
        bblog::error("[Input] Analog action \"{}\" is not bound to any input, couldn't find any glyph path", actionName);
        return {};
    }

    for (uint32_t i = 0; i < STEAM_INPUT_MAX_ORIGINS; ++i)
    {
        if (origins[i] == k_EInputActionOrigin_None)
        {
            break;
        }

        glyphPaths.push_back(SteamInput()->GetGlyphPNGForActionOrigin(origins[i], k_ESteamInputGlyphSize_Large, 0));
    }

    return glyphPaths;
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
