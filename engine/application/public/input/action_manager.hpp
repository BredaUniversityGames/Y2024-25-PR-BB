#pragma once
#include "input_device_manager.hpp"
#include <variant>

class InputDeviceManager;

enum class DigitalActionType : uint8_t
{
    // Action happens once when input is received.
    ePressed,
    // Action happens continuously when input is being received.
    eHold,
    // Action happens once when input is released.
    eReleased,
};

using DigitalInputAction = std::variant<GamepadButton, KeyboardCode, MouseButton>;
using AnalogInputAction = GamepadAnalog;

using DigitalInputActionList = std::vector<DigitalInputAction>;
using AnalogInputActionList = std::vector<AnalogInputAction>;

// Action for button inputs.
struct DigitalAction
{
    std::string name {};
    DigitalActionType type {};
    DigitalInputActionList inputs {};
};

// Action for axis inputs.
struct AnalogAction
{
    std::string name {};
    AnalogInputActionList inputs {};
};

// A collection of actions for a game mode. (game mode -> how a player should play in a situation, e.g. main menu or gameplay)
struct ActionSet
{
    std::string name {};
    std::vector<DigitalAction> digitalActions {};
    std::vector<AnalogAction> analogActions {};
};

// A collection of action sets that contain all actions across the game.
using GameActions = std::vector<ActionSet>;

// Abstract class, which manages keyboard and mouse actions.
// Inherit to handle gamepad actions.
class ActionManager
{
public:
    ActionManager(const InputDeviceManager& inputDeviceManager);
    virtual ~ActionManager() = default;

    virtual void Update() { }

    // Sets the actions in the game, the first set in the game actions is assumed to be the used set.
    virtual void SetGameActions(const GameActions& gameActions) { _gameActions = gameActions; }
    // Change the currently used action set in the game actions.
    virtual void SetActiveActionSet(std::string_view actionSetName);

    // Button actions.
    [[nodiscard]] bool GetDigitalAction(std::string_view actionName) const;
    // Axis actions.
    virtual void GetAnalogAction(std::string_view actionName, float& x, float& y) const = 0;

protected:
    const InputDeviceManager& _inputDeviceManager;
    GameActions _gameActions {};
    uint32_t _activeActionSet = 0;

    [[nodiscard]] bool CheckDigitalInput(const DigitalAction& action) const;
    [[nodiscard]] bool CheckInput(MAYBE_UNUSED std::string_view actionName, KeyboardCode code, DigitalActionType inputType) const;
    [[nodiscard]] bool CheckInput(MAYBE_UNUSED std::string_view actionName, MouseButton button, DigitalActionType inputType) const;
    [[nodiscard]] virtual bool CheckInput(std::string_view actionName, GamepadButton button, DigitalActionType inputType) const = 0;
};