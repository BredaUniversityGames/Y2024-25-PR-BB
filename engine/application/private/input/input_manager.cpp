#include "input/input_manager.hpp"
#include "log.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace detail
{
template <typename K, typename V>
V UnorderedMapGetOr(const std::unordered_map<K, V>& map, const K& key, const V& defaultValue)
{
    if (auto it = map.find(key); it != map.end())
    {
        return it->second;
    }
    return defaultValue;
}
}

InputManager::InputManager()
{
    SDL_GetMouseState(&_mouse.positionX, &_mouse.positionY);
}

InputManager::~InputManager()
{
    if (IsGamepadAvailable())
    {
        CloseGamepad();
    }
}

void InputManager::Update()
{
    for (auto& key : _keyboard.inputPressed)
        key.second = false;
    for (auto& key : _keyboard.inputReleased)
        key.second = false;

    for (auto& button : _mouse.inputPressed)
        button.second = false;
    for (auto& button : _mouse.inputReleased)
        button.second = false;

    if (IsGamepadAvailable())
    {
        for (auto& button : _gamepad.inputPressed)
            button.second = false;
        for (auto& button : _gamepad.inputReleased)
            button.second = false;
    }
}

void InputManager::UpdateEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
        if (event.key.repeat == 0)
        { // Only process on first keydown, not when holding
            KeyboardCode key = static_cast<KeyboardCode>(event.key.key);
            _keyboard.inputPressed[key] = true;
            _keyboard.inputHeld[key] = true;
        }
        break;
    case SDL_EVENT_KEY_UP:
    {
        KeyboardCode key = static_cast<KeyboardCode>(event.key.key);
        _keyboard.inputHeld[key] = false;
        _keyboard.inputReleased[key] = true;
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        _mouse.inputPressed[button] = true;
        _mouse.inputHeld[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        _mouse.inputHeld[button] = false;
        _mouse.inputReleased[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        _mouse.positionX += event.motion.xrel;
        _mouse.positionY += event.motion.yrel;
        break;

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
    default:
        break;
    }
}

bool InputManager::IsKeyPressed(KeyboardCode key) const
{
    return detail::UnorderedMapGetOr(_keyboard.inputPressed, key, false);
}

bool InputManager::IsKeyHeld(KeyboardCode key) const
{
    return detail::UnorderedMapGetOr(_keyboard.inputHeld, key, false);
}

bool InputManager::IsKeyReleased(KeyboardCode key) const
{
    return detail::UnorderedMapGetOr(_keyboard.inputReleased, key, false);
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    return detail::UnorderedMapGetOr(_mouse.inputPressed, button, false);
}

bool InputManager::IsMouseButtonHeld(MouseButton button) const
{
    return detail::UnorderedMapGetOr(_mouse.inputHeld, button, false);
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const
{
    return detail::UnorderedMapGetOr(_mouse.inputReleased, button, false);
}

void InputManager::GetMousePosition(int32_t& x, int32_t& y) const
{
    x = static_cast<int32_t>(_mouse.positionX);
    y = static_cast<int32_t>(_mouse.positionY);
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button) const
{
    if (!IsGamepadAvailable())
    {
        return false;
    }

    return detail::UnorderedMapGetOr(_gamepad.inputPressed, button, false);
}

bool InputManager::IsGamepadButtonHeld(GamepadButton button) const
{
    if (!IsGamepadAvailable())
    {
        return false;
    }

    return detail::UnorderedMapGetOr(_gamepad.inputHeld, button, false);
}

bool InputManager::IsGamepadButtonReleased(GamepadButton button) const
{
    if (!IsGamepadAvailable())
    {
        return false;
    }

    return detail::UnorderedMapGetOr(_gamepad.inputReleased, button, false);
}

float InputManager::GetGamepadAxis(GamepadAxis axis) const
{
    if (!IsGamepadAvailable())
    {
        return 0.0f;
    }

    float value = GetRawGamepadAxis(axis);

    if (axis == GamepadAxis::eGAMEPAD_AXIS_LEFT_TRIGGER || axis == GamepadAxis::eGAMEPAD_AXIS_RIGHT_TRIGGER)
    {
        return value;
    }

    return ClampDeadzone(value, INNER_GAMEPAD_DEADZONE, OUTER_GAMEPAD_DEADZONE);
}

float InputManager::GetRawGamepadAxis(GamepadAxis axis) const
{
    if (!IsGamepadAvailable())
    {
        return 0.0f;
    }

    SDL_GamepadAxis sdlAxis = static_cast<SDL_GamepadAxis>(axis);
    float value = SDL_GetGamepadAxis(_gamepad.sdlHandle, sdlAxis);
    value /= SDL_JOYSTICK_AXIS_MAX; // Convert to -1 to 1

    return value;
}

float InputManager::ClampDeadzone(float input, float innerDeadzone, float outerDeadzone) const
{
    if (glm::abs(input) < innerDeadzone)
    {
        return 0.0f;
    }

    if (glm::abs(input) > outerDeadzone)
    {
        return input < 0.0f ? MIN_GAMEPAD_AXIS : MAX_GAMEPAD_AXIS;
    }

    return input;
}

void InputManager::CloseGamepad()
{
    SDL_CloseGamepad(_gamepad.sdlHandle);
    _gamepad.sdlHandle = nullptr;
}