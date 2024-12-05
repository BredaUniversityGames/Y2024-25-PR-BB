#include "input/action_manager.hpp"

// TODO: Custom include header
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <steam_api.h>
#pragma clang diagnostic pop

class SteamActionManager final : public ActionManager
{
public:
    SteamActionManager(const InputManager& inputManager);
    virtual ~SteamActionManager() final = default;

    virtual void Update() final;
    virtual void SetGameActions(const GameActions& gameActions) final;

    [[nodiscard]] virtual bool GetDigitalAction(std::string_view actionName) const final;
    void GetAnalogAction(std::string_view actionName, float &x, float &y) const final;

private:
    InputHandle_t _inputHandle {};

    struct SteamActionSetCache
    {
        InputActionSetHandle_t actionSetHandle {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadDigitalActionsCache {};
        std::unordered_map<std::string, InputDigitalActionHandle_t> gamepadAnalogActionsCache {};
    };

    std::vector<SteamActionSetCache> _steamActionSetCache {};
};