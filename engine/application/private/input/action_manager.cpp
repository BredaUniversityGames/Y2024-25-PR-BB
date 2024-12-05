#include "input/action_manager.hpp"

ActionManager::ActionManager()
{
    if (!SteamInput()->Init(false))
    {
        bblog::error("[Steamworks] Failed to initialize Steam Input");
        return;
    }

    GameActions gameActions{};

    ActionSet& actionSet = gameActions.emplace_back();
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

    SetGameActions(gameActions);
}

void ActionManager::Update()
{
    // Find active controllers this frame
    InputHandle_t pHandles[STEAM_CONTROLLER_MAX_COUNT];
    int nNumActive = SteamInput()->GetConnectedControllers(pHandles);

    // If there's an active controller, and if we're not already using it, select the first one.
    if (nNumActive && (_inputHandle != pHandles[0]))
    {
        _inputHandle = pHandles[ 0 ];
    }

    if (_inputHandle == 0 || _gameActions.empty())
    {
        return;
    }

    // Set action set for the controller
    SteamInput()->ActivateActionSet(_inputHandle, _steamActionSetCache[_activeActionSet].actionSetHandle);
}

void ActionManager::SetGameActions(const GameActions& gameActions)
{
    _gameActions = gameActions;

    // Caching Steam Input API handles
    _steamActionSetCache.resize(_gameActions.size());

    for (uint32_t i = 0; i < _gameActions.size(); ++i)
    {
        _steamActionSetCache[i].actionSetHandle = SteamInput()->GetActionSetHandle(_gameActions[i].name.c_str());

        for (const DigitalAction& action : _gameActions[i].digitalActions)
        {
            _steamActionSetCache[i].gamepadDigitalActionsCache.emplace(action.name, SteamInput()->GetDigitalActionHandle(action.name.c_str()));
        }

        for (const AnalogAction& action : _gameActions[i].analogActions)
        {
            _steamActionSetCache[i].gamepadAnalogActionsCache.emplace(action.name, SteamInput()->GetAnalogActionHandle(action.name.c_str()));
        }
    }
}

void ActionManager::SetActiveActionSet(std::string_view actionSetName)
{
    auto itr = std::find_if(
    _gameActions.begin(), _gameActions.end(),
    [actionSetName](const ActionSet& actionSet) { return actionSet.name == actionSetName;});

    if (itr == _gameActions.end())
    {
        bblog::error("[Input] Failed to find action set: \"{}\"", actionSetName);
        return;
    }

    uint32_t index = itr - _gameActions.begin();
    _activeActionSet = index;
}

bool ActionManager::GetDigitalAction(std::string_view actionName) const
{
    if (_inputHandle == 0 || _gameActions.empty())
    {
        return false;
    }

    const SteamActionSetCache& actionSetCache = _steamActionSetCache[_activeActionSet];

    auto itr = actionSetCache.gamepadDigitalActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadDigitalActionsCache.end())
    {
        bblog::error("[Input] Failed to find digital action: \"{}\"", actionName);
        return false;
    }

    ControllerDigitalActionData_t digitalData = SteamInput()->GetDigitalActionData(_inputHandle, itr->second);

    // Actions are only 'active' when they're assigned to a control in an action set, and that action set is active.
    if (!digitalData.bActive)
    {
        return false;
    }

    return digitalData.bState;
}

void ActionManager::GetAnalogAction(std::string_view actionName, float& x, float& y) const
{
    if (_inputHandle == 0 || _gameActions.empty())
    {
        return;
    }

    const SteamActionSetCache& actionSetCache = _steamActionSetCache[_activeActionSet];

    auto itr = actionSetCache.gamepadAnalogActionsCache.find(actionName.data());
    if (itr == actionSetCache.gamepadAnalogActionsCache.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return;
    }

    ControllerAnalogActionData_t analogActionData = SteamInput()->GetAnalogActionData(_inputHandle, itr->second);

    // Actions are only 'active' when they're assigned to a control in an action set, and that action set is active.
    if (!analogActionData.bActive)
    {
        return;
    }

    x = analogActionData.x;
    y = analogActionData.y;
}