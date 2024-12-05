#include "input/action_manager.hpp"

ActionManager::ActionManager()
{
    if (!SteamInput()->Init(false))
    {
        bblog::error("[Steamworks] Failed to initialize Steam Input");
        return;
    }

    ActionSet actionSet{};
    actionSet.name = "FlyCamera";

    DigitalAction exitAction{};
    exitAction.name = "Exit";

    AnalogAction moveAction{};
    moveAction.name = "Move";

    AnalogAction cameraAction{};
    cameraAction.name = "Camera";

    actionSet.digitalActions.push_back(exitAction);
    actionSet.analogActions.push_back(moveAction);
    actionSet.analogActions.push_back(cameraAction);

    SetActionSet(actionSet);
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
    SteamInput()->ActivateActionSet(inputHandle, _actionSetHandle);

    bool a = GetDigitalAction("Exit");
    if (a)
    {
        bblog::info("Exit: {}", a);
    }
}

void ActionManager::SetActionSet(const ActionSet& actionSet)
{
    _actionSet = actionSet;

    // Caching Steam Input API handles
    _actionSetHandle = SteamInput()->GetActionSetHandle(_actionSet.name.c_str());

    for (const DigitalAction& action : _actionSet.digitalActions)
    {
        _gamepadDigitalActionsCache.emplace(action.name, SteamInput()->GetDigitalActionHandle(action.name.c_str()));
    }

    for (const AnalogAction& action : _actionSet.analogActions)
    {
        _gamepadAnalogActionsCache.emplace(action.name, SteamInput()->GetAnalogActionHandle(action.name.c_str()));
    }
}

bool ActionManager::GetDigitalAction(std::string_view actionName) const
{
    if (inputHandle == 0)
    {
        return false;
    }

    auto itr = _gamepadDigitalActionsCache.find(actionName.data());
    if (itr == _gamepadDigitalActionsCache.end())
    {
        bblog::error("[Input] Failed to find digital action: \"{}\"", actionName);
        return false;
    }

    ControllerDigitalActionData_t digitalData = SteamInput()->GetDigitalActionData(inputHandle, itr->second);

    // Actions are only 'active' when they're assigned to a control in an action set, and that action set is active.
    if (!digitalData.bActive)
    {
        return false;
    }

    return digitalData.bState;
}