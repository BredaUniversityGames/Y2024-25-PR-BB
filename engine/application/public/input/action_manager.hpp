#pragma once
#include "common.hpp"
#include "enum_utils.hpp"
#include "input_device_manager.hpp"
#include <glm/glm.hpp>
#include <string>
#include <variant>
#include <vector>

class InputDeviceManager;

enum class DigitalActionType : uint8_t
{
    eNone = 0 << 0,
    // Action happens once when input is received.
    ePressed = 1 << 0,
    // Action happens continuously when input is being received.
    eHeld = 1 << 1,
    // Action happens once when input is released.
    eReleased = 1 << 2,
};
GENERATE_ENUM_FLAG_OPERATORS(DigitalActionType)

struct KeyboardAnalog
{
    KeyboardCode up;
    KeyboardCode down;
    KeyboardCode left;
    KeyboardCode right;
};

using DigitalInputBinding = std::variant<GamepadButton, KeyboardCode, MouseButton>;
using AnalogInputBinding = std::variant<GamepadAnalog, KeyboardAnalog>;

// Action for button inputs.
struct DigitalAction
{
    std::string name {};
    std::vector<DigitalInputBinding> inputs {};
};

// Action for axis inputs.
struct AnalogAction
{
    std::string name {};
    std::vector<AnalogInputBinding> inputs {};
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

// Utility wrapper for easy action state checking.
struct DigitalActionResult
{
    // True once upon pressed.
    bool IsPressed() const { return HasAnyFlags(value, DigitalActionType::ePressed); }
    // True all the time upon held.
    bool IsHeld() const { return HasAnyFlags(value, DigitalActionType::eHeld); }
    // True once upon released.
    bool IsReleased() const { return HasAnyFlags(value, DigitalActionType::eReleased); }

    DigitalActionType value = DigitalActionType::eNone;
};

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
    NO_DISCARD DigitalActionResult GetDigitalAction(std::string_view actionName) const;

    // Axis actions.
    NO_DISCARD glm::vec2 GetAnalogAction(std::string_view actionName) const;

protected:
    const InputDeviceManager& _inputDeviceManager;
    GameActions _gameActions {};
    uint32_t _activeActionSet = 0;

    NO_DISCARD DigitalActionType CheckDigitalInput(const DigitalAction& action) const;
    NO_DISCARD DigitalActionType CheckInput(MAYBE_UNUSED std::string_view actionName, KeyboardCode code) const;
    NO_DISCARD DigitalActionType CheckInput(MAYBE_UNUSED std::string_view actionName, MouseButton button) const;
    NO_DISCARD virtual DigitalActionType CheckInput(std::string_view actionName, GamepadButton button) const = 0;
    NO_DISCARD glm::vec2 CheckAnalogInput(const AnalogAction& action) const;
    NO_DISCARD glm::vec2 CheckInput(MAYBE_UNUSED std::string_view actionName, const KeyboardAnalog& keyboardAnalog) const;
    NO_DISCARD virtual glm::vec2 CheckInput(std::string_view actionName, GamepadAnalog gamepadAnalog) const = 0;
};