#include "input/sdl/sdl_input_device_manager.hpp"
#include "hashmap_utils.hpp"
#include "log.hpp"
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

        // SDL treats triggers as axis inputs, but Steam treats them as button inputs, so to stay consistent we also treat triggers as buttons
        UpdateGamepadTriggerButtons();
    }
}

void SDLInputDeviceManager::UpdateEvent(const SDL_Event& event)
{
    InputDeviceManager::UpdateEvent(event);

    switch (event.type)
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

GamepadType SDLInputDeviceManager::GetGamepadType() const
{
    auto ConvertGamepadType = [](SDL_GamepadType sdlType) -> GamepadType
    {
        switch (sdlType)
        {
        case SDL_GAMEPAD_TYPE_STANDARD:
            return GamepadType::eGenericXInput;
        case SDL_GAMEPAD_TYPE_XBOX360:
            return GamepadType::eXBox360Controller;
        case SDL_GAMEPAD_TYPE_XBOXONE:
            return GamepadType::eXBoxOneController;
        case SDL_GAMEPAD_TYPE_PS3:
            return GamepadType::ePS3Controller;
        case SDL_GAMEPAD_TYPE_PS4:
            return GamepadType::ePS4Controller;
        case SDL_GAMEPAD_TYPE_PS5:
            return GamepadType::ePS5Controller;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
            return GamepadType::eSwitchProController;
        default:
            return GamepadType::eUnknown;
        }
    };

    if (!IsGamepadAvailable())
    {
        bblog::error("[Input] No gamepad available while trying to get it's type!");
        return GamepadType::eUnknown;
    }

    const SDL_GamepadType type = SDL_GetGamepadType(_gamepad.sdlHandle);
    return ConvertGamepadType(type);
}

void SDLInputDeviceManager::CloseGamepad()
{
    SDL_CloseGamepad(_gamepad.sdlHandle);
    _gamepad.sdlHandle = nullptr;
}

void SDLInputDeviceManager::UpdateGamepadTriggerButtons()
{
    bool currentLeftTriggerState = GetGamepadAxis(GamepadAxis::eLEFT_TRIGGER) != 0.0f;
    bool currentRightTriggerState = GetGamepadAxis(GamepadAxis::eRIGHT_TRIGGER) != 0.0f;

    _gamepad.inputPressed[GamepadButton::eLEFT_TRIGGER] = currentLeftTriggerState && !_gamepad.prevLeftTriggerState;
    _gamepad.inputHeld[GamepadButton::eLEFT_TRIGGER] = currentLeftTriggerState;
    _gamepad.inputReleased[GamepadButton::eLEFT_TRIGGER] = !currentLeftTriggerState && _gamepad.prevLeftTriggerState;

    _gamepad.inputPressed[GamepadButton::eRIGHT_TRIGGER] = currentRightTriggerState && !_gamepad.prevRightTriggerState;
    _gamepad.inputHeld[GamepadButton::eRIGHT_TRIGGER] = currentRightTriggerState;
    _gamepad.inputReleased[GamepadButton::eRIGHT_TRIGGER] = !currentRightTriggerState && _gamepad.prevRightTriggerState;

    _gamepad.prevLeftTriggerState = currentLeftTriggerState;
    _gamepad.prevRightTriggerState = currentRightTriggerState;
}
