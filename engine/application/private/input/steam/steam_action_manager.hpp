#pragma once
#include "input/action_manager.hpp"
#include "steam_include.hpp"

class SteamInputDeviceManager;

class SteamActionManager final : public ActionManager
{
public:
    SteamActionManager(const SteamInputDeviceManager& steamInputDeviceManager);
    virtual ~SteamActionManager() final = default;

    virtual void Update() final;
    virtual void SetGameActions(const GameActions& gameActions) final;

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

    [[nodiscard]] bool CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button, DigitalActionType inputType) const;
    [[nodiscard]] virtual glm::vec2 CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadAnalog gamepadAnalog) const final;
    void UpdateSteamControllerInputState();
};