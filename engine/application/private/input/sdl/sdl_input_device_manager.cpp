#include "input/sdl/sdl_input_device_manager.hpp"
#include "log.hpp"
#include "hashmap_utils.hpp"
#include <SDL3/SDL.h>

SDLInputDeviceManager::SDLInputDeviceManager()
    : InputDeviceManager()
{
}

SDLInputDeviceManager::~SDLInputDeviceManager()
{
    if (IsGamepadAvailable())
    {
        CloseGamepad();
    }
}

void SDLInputDeviceManager::Update()
{
    InputDeviceManager::Update();

    if (IsGamepadAvailable())
    {
        for (auto& button : _gamepad.inputPressed)
            button.second = false;
        for (auto& button : _gamepad.inputReleased)
            button.second = false;
    }
}

void SDLInputDeviceManager::UpdateEvent(const SDL_Event& event)
{
    InputDeviceManager::UpdateEvent(event);

    switch(event.type)
    {
    case SDL_EVENT_GAMEPAD_ADDED:
    {
        if (!IsGamepadAvailable())
        {
            _gamepad.sdlHandle = SDL_OpenGamepad(event.gdevice.which);
            bblog::info("[INPUT] SDL gamepad device added.");
        }
        break;
    }
    case SDL_EVENT_GAMEPAD_REMOVED:
    {
        if (IsGamepadAvailable())
        {
            if (SDL_GetGamepadID(_gamepad.sdlHandle) == event.gdevice.which)
            {
                CloseGamepad();
                bblog::info("[INPUT] SDL gamepad device removed.");
            }
        }
        break;
    }

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    {
        GamepadButton button = static_cast<GamepadButton>(event.jbutton.button);
        _gamepad.inputPressed[button] = true;
        _gamepad.inputHeld[button] = true;
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    {
        GamepadButton button = static_cast<GamepadButton>(event.jbutton.button);
        _gamepad.inputHeld[button] = false;
        _gamepad.inputReleased[button] = true;
        break;
    }

    default:
        break;
    }
}

bool SDLInputDeviceManager::IsGamepadButtonPressed(GamepadButton button) const
{
    if (!IsGamepadAvailable())
    {
        return false;
    }

    return UnorderedMapGetOr(_gamepad.inputPressed, button, false);
}

bool SDLInputDeviceManager::IsGamepadButtonHeld(GamepadButton button) const
{
    if (!IsGamepadAvailable())
    {
        return false;
    }

    return UnorderedMapGetOr(_gamepad.inputHeld, button, false);
}

bool SDLInputDeviceManager::IsGamepadButtonReleased(GamepadButton button) const
{
    if (!IsGamepadAvailable())
    {
        return false;
    }

    return UnorderedMapGetOr(_gamepad.inputReleased, button, false);
}

float SDLInputDeviceManager::GetGamepadAxis(GamepadAxis axis) const
{
    if (!IsGamepadAvailable())
    {
        return 0.0f;
    }

    float value = GetRawGamepadAxis(axis);

    if (axis == GamepadAxis::eLEFT_TRIGGER || axis == GamepadAxis::eRIGHT_TRIGGER)
    {
        return value;
    }

    return ClampGamepadAxisDeadzone(value);
}

float SDLInputDeviceManager::GetRawGamepadAxis(GamepadAxis axis) const
{
    if (!IsGamepadAvailable())
    {
        return 0.0f;
    }

    SDL_GamepadAxis sdlAxis = static_cast<SDL_GamepadAxis>(axis);
    float value = SDL_GetGamepadAxis(_gamepad.sdlHandle, sdlAxis);
    value /= SDL_JOYSTICK_AXIS_MAX; // Convert to -1 to 1

    // SDL Y axis is inverted (-1 is up and 1 is down), so we invert it
    if (axis == GamepadAxis::eLEFTY || axis == GamepadAxis::eRIGHTY)
    {
        value *= -1.0f;
    }

    return value;
}

void SDLInputDeviceManager::CloseGamepad()
{
    SDL_CloseGamepad(_gamepad.sdlHandle);
    _gamepad.sdlHandle = nullptr;
}