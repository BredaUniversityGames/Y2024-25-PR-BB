#pragma once
#include "input/action_manager.hpp"
#include "steam_include.hpp"

class SteamActionManager final : public ActionManager
{
public:
    SteamActionManager(const InputManager& inputManager);
    virtual ~SteamActionManager() final = default;

    virtual void Update() final;
    virtual void SetGameActions(const GameActions& gameActions) final;

    virtual void GetAnalogAction(std::string_view actionName, float &x, float &y) const final;

private:
    InputHandle_t _inputHandle {};

    struct SteamActionSetCache
    {
        InputActionSetHandle_t actionSetHandle {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadDigitalActionsCache {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadAnalogActionsCache {};
    };

    using SteamGameActionsCache = std::vector<SteamActionSetCache>;
     SteamGameActionsCache _steamGameActionsCache {};

    // We have to track pressed and released inputs ourselves as steam input API doesn't do it for us, so we save the current and previous frames input states
    using SteamControllerState = std::unordered_map<std::string, bool>;
    SteamControllerState _currentControllerState {};
    SteamControllerState _prevControllerState {};

    [[nodiscard]] virtual bool CheckDigitalInput(const DigitalAction& action) const final;
    [[nodiscard]] bool CheckInput(std::string_view actionName, MAYBE_UNUSED GamepadButton button, DigitalInputActionType inputType) const;
    [[nodiscard]] bool CheckInput(MAYBE_UNUSED std::string_view actionName, KeyboardCode code, DigitalInputActionType inputType) const;
    [[nodiscard]] bool CheckInput(MAYBE_UNUSED std::string_view actionName, MouseButton button, DigitalInputActionType inputType) const;

    void UpdateActiveController();
    void UpdateSteamControllerInputState();
    [[nodiscard]] bool IsControllerAvailable() const { return _inputHandle != 0; }
};