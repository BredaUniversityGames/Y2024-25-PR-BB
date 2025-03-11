#include "input/steam/steam_input_device_manager.hpp"
#include <array>

SteamInputDeviceManager::SteamInputDeviceManager()
    : InputDeviceManager()
{
    // Update Steam Input to make sure we have the latest controller connectivity.
    // Otherwise, on the first frame, Steam Input doesn't have any data about controller connectivity.
    SteamInput()->RunFrame();

    UpdateControllerConnectivity();
}

void SteamInputDeviceManager::Update()
{
    InputDeviceManager::Update();
    UpdateControllerConnectivity();
}

void SteamInputDeviceManager::UpdateControllerConnectivity()
{
    std::array<InputHandle_t, STEAM_CONTROLLER_MAX_COUNT> handles {};
    int numActive = SteamInput()->GetConnectedControllers(handles.data());

    if (numActive == 0)
    {
        _inputHandle = 0;
        return;
    }

    if (_inputHandle != handles[0])
    {
        _inputHandle = handles[0];
    }
}
