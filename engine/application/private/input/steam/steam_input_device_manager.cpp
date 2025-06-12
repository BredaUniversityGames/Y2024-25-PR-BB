#include "input/steam/steam_input_device_manager.hpp"
#include "log.hpp"
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

GamepadType SteamInputDeviceManager::GetGamepadType() const
{
    auto ConvertGamepadType = [](ESteamInputType steamType) -> GamepadType
    {
        switch (steamType)
        {
        case k_ESteamInputType_SteamController:
            return GamepadType::eSteamController;
        case k_ESteamInputType_XBox360Controller:
            return GamepadType::eXBox360Controller;
        case k_ESteamInputType_XBoxOneController:
            return GamepadType::eXBoxOneController;
        case k_ESteamInputType_GenericGamepad:
            return GamepadType::eGenericXInput;
        case k_ESteamInputType_PS4Controller:
            return GamepadType::ePS4Controller;
        case k_ESteamInputType_PS5Controller:
            return GamepadType::ePS5Controller;
        case k_ESteamInputType_SwitchProController:
            return GamepadType::eSwitchProController;
        case k_ESteamInputType_MobileTouch:
            return GamepadType::eMobileTouch;
        case k_ESteamInputType_PS3Controller:
            return GamepadType::ePS3Controller;
        default:
            return GamepadType::eUnknown;
        }
    };

    if (!IsGamepadAvailable())
    {
        bblog::error("[Input] No gamepad available while trying to get it's type!");
        return GamepadType::eUnknown;
    }

    const ESteamInputType type = SteamInput()->GetInputTypeForHandle(_inputHandle);
    return ConvertGamepadType(type);
}

void SteamInputDeviceManager::UpdateControllerConnectivity()
{
    std::array<InputHandle_t, STEAM_CONTROLLER_MAX_COUNT> handles {};
    int numActive = SteamInput()->GetConnectedControllers(handles.data());

    if (numActive == 0)
    {
        if (_inputHandle != 0)
        {
            bblog::info("[Input] Steam gamepad device removed");
        }

        _inputHandle = 0;
        return;
    }

    if (_inputHandle != handles[0])
    {
        _inputHandle = handles[0];
        bblog::info("[Input] Steam gamepad device added");
    }
}
