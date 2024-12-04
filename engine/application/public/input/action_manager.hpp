#pragma once

// TODO: Custom include header
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <steam_api.h>
#pragma clang diagnostic pop

class ActionManager
{
public:
    ActionManager();
    ~ActionManager() = default;

    void Update();

    InputDigitalActionHandle_t exitActionHandle;

    InputAnalogActionHandle_t moveActionHandle;
    InputAnalogActionHandle_t cameraActionHandle;

    InputActionSetHandle_t actionSetHandle;

    InputHandle_t inputHandle;
};