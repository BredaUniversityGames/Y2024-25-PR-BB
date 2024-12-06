#pragma once
#include "input/input_codes/gamepad.hpp"
#include "input/input_codes/keys.hpp"
#include "input/input_codes/mousebuttons.hpp"

#include <cstdint>
#include <variant>

class InputManager;

enum class DigitalInputActionType : uint8_t
{
    Pressed,
    Hold,
    Released,
};

struct DigitalInputAction
{
    DigitalInputActionType type{};
    std::variant<GamepadButton, KeyboardCode, MouseButton> code;
};

using AnalogInputAction = GamepadAnalog;

using DigitalInputActionList = std::vector<DigitalInputAction>;
using AnalogInputActionList = std::vector<AnalogInputAction>;

struct DigitalAction
{
    std::string name{};
    DigitalInputActionList inputs{};
};

struct AnalogAction
{
    std::string name{};
    AnalogInputActionList inputs{};
};

struct ActionSet
{
    std::string name {};
    std::vector<DigitalAction> digitalActions {};
    std::vector<AnalogAction> analogActions {};
};

using GameActions = std::vector<ActionSet>;

class ActionManager
{
public:
    ActionManager(const InputManager& inputManager);
    virtual ~ActionManager() = default;

    virtual void Update(){}
    // Sets the actions in the game, the first set in the game actions is assumed to be the used set.
    virtual void SetGameActions(const GameActions& gameActions);
    // Change the currently used action set in the game actions.
    void SetActiveActionSet(std::string_view actionSetName);

    [[nodiscard]] bool GetDigitalAction(std::string_view actionName) const;
    virtual void GetAnalogAction(std::string_view actionName, float& x, float& y) const;

protected:
    const InputManager& _inputManager;
    GameActions _gameActions;
    uint32_t _activeActionSet = 0;

    [[nodiscard]] virtual bool CheckDigitalInput(const DigitalAction& action) const;
    void CheckAnalogInput(const AnalogAction& action, float& x, float& y) const;
    [[nodiscard]] bool CheckInput(KeyboardCode code, DigitalInputActionType inputType) const;
    [[nodiscard]] bool CheckInput(MouseButton button, DigitalInputActionType inputType) const;

private:
    [[nodiscard]] bool CheckInput(GamepadButton button, DigitalInputActionType inputType) const;
};