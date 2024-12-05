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
    ActionManager();
    ~ActionManager() = default;

    void Update();
    void SetGameActions(const GameActions& gameActions);
    void SetActiveActionSet(std::string_view actionSetName);

    [[nodiscard]] bool GetDigitalAction(std::string_view actionName) const;
    void GetAnalogAction(std::string_view actionName, float& x, float& y) const;

private:
    GameActions _gameActions;
    uint32_t _activeActionSet = 0;

    // Steam specifics
    InputHandle_t _inputHandle;

    struct SteamActionSetCache
    {
        InputActionSetHandle_t actionSetHandle;
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadDigitalActionsCache {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadAnalogActionsCache {};
    };

    std::vector<SteamActionSetCache> _steamActionSetCache;
};