#include "input/steam_input_device_manager.hpp"

void SteamInputDeviceManager::Update()
{
    InputDeviceManager::Update();

    std::array<InputHandle_t, STEAM_CONTROLLER_MAX_COUNT> handles {};
    int numActive = SteamInput()->GetConnectedControllers(handles.data());

    // If there's an active controller, and if we're not already using it, select the first one.
    if (numActive && (_inputHandle != handles[0]))
    {
        _inputHandle = handles[0];
    }
}