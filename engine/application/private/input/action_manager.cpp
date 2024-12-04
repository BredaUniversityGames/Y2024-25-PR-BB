#include "input/action_manager.hpp"

ActionManager::ActionManager()
{
    if (!SteamInput()->Init(false))
    {
        bblog::error("[Steamworks] Failed to initialize Steam Input");
        return;
    }

    exitActionHandle = SteamInput()->GetDigitalActionHandle("Exit");

    moveActionHandle = SteamInput()->GetAnalogActionHandle("Move");
    cameraActionHandle = SteamInput()->GetAnalogActionHandle("Camera");

    actionSetHandle = SteamInput()->GetActionSetHandle("FlyCamera");
}

void ActionManager::Update()
{
    // Find active controllers this frame
    InputHandle_t pHandles[STEAM_CONTROLLER_MAX_COUNT];
    int nNumActive = SteamInput()->GetConnectedControllers(pHandles);

    // If there's an active controller, and if we're not already using it, select the first one.
    if (nNumActive && (inputHandle != pHandles[0]))
    {
        inputHandle = pHandles[ 0 ];
    }

    if (inputHandle == 0)
    {
        return;
    }

    // Set action set for the controller
    SteamInput()->ActivateActionSet(inputHandle, actionSetHandle);

    ControllerDigitalActionData_t digitalData = SteamInput()->GetDigitalActionData(inputHandle, exitActionHandle);

    // Actions are only 'active' when they're assigned to a control in an action set, and that action set is active.
    if (digitalData.bActive)
    {
        if (digitalData.bState)
        {
            bblog::info("pressed!");
        }
    }

}