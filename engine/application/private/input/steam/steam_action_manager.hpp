#pragma once
#include "input/action_manager.hpp"
#include "steam_include.hpp"

class SteamInputDeviceManager;

class SteamActionManager final : public ActionManager
{
public:
    SteamActionManager(const SteamInputDeviceManager& steamInputDeviceManager);
    ~SteamActionManager() final = default;

    void Update() final;
    void SetGameActions(const GameActions& gameActions) final;

    // Returns information to be visually displayed for all gamepad bindings for the given digital action.
    NO_DISCARD std::vector<GamepadOriginVisual> GetDigitalActionGamepadOriginVisual(std::string_view actionName) const final;

    // Returns information to be visually displayed for all gamepad bindings for the given analog action.
    NO_DISCARD std::vector<GamepadOriginVisual> GetAnalogActionGamepadOriginVisual(std::string_view actionName) const final;

private:
    const SteamInputDeviceManager& _steamInputDeviceManager;

    struct SteamActionSetCache
    {
        InputActionSetHandle_t actionSetHandle {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadDigitalActionsCache {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadAnalogActionsCache {};
    };

    using SteamGameActionsCache = std::vector<SteamActionSetCache>;
    SteamGameActionsCache _steamGameActionsCache {};

    // We have to track pressed and released inputs ourselves as steam input API doesn't do it for us properly,
    // so we save the current and previous frames input states per action.
    using SteamControllerState = std::unordered_map<std::string, bool>;
    SteamControllerState _currentControllerState {};
    SteamControllerState _prevControllerState {};

    NO_DISCARD DigitalActionType CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button) const;
    NO_DISCARD glm::vec2 CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadAnalog gamepadAnalog) const final;
    void UpdateSteamControllerInputState();
    void CacheSteamInputHandles();
};
