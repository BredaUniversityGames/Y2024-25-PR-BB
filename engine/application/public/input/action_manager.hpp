#pragma once

// TODO: Custom include header
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <steam_api.h>
#pragma clang diagnostic pop

#include <variant>

#include "input/input_codes/gamepad.hpp"
#include "input/input_codes/keys.hpp"
#include "input/input_codes/mousebuttons.hpp"

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

using AnalogInputAction = GamepadAxis;

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

struct ActionMappingTable
{
    std::string name {};
    std::vector<DigitalAction> digitalActions {};
    std::vector<AnalogAction> analogActions {};
};

class ActionManager
{
public:
    ActionManager();
    ~ActionManager() = default;

    void Update();
    void SetActionMappingTable(const ActionMappingTable& actionMappingTable);

    [[nodiscard]] bool GetDigitalAction(std::string_view actionName) const;

private:
    ActionMappingTable _actionMappingTable;

    // Steam specifics
    InputHandle_t inputHandle;
    InputActionSetHandle_t _actionSetHandle;
    std::unordered_map<std::string, InputDigitalActionHandle_t> _gamepadDigitalActionsCache {};
    std::unordered_map<std::string, InputDigitalActionHandle_t> _gamepadAnalogActionsCache {};
};