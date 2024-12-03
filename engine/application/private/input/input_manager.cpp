#include "input/input_manager.hpp"
#include "log.hpp"

#include <SDL3/SDL.h>

namespace detail
{
template <typename K, typename V>
V unordered_map_get_or(const std::unordered_map<K, V>& map, const K& key, const V& default_value)
{
    if (auto it = map.find(key); it != map.end())
    {
        return it->second;
    }
    return default_value;
}
}

InputManager::InputManager()
{
    SDL_GetMouseState(&_mouse.positionX, &_mouse.positionY);

    int numGamepads{};
    SDL_JoystickID* joySticks = SDL_GetGamepads(&numGamepads);

    for (int i = 0; i < numGamepads; i++)
    {
        if (SDL_IsGamepad(joySticks[i]))
        {
            _gamepad.sdlHandle = SDL_OpenGamepad(joySticks[i]);
            break;
        }
    }
}

InputManager::~InputManager()
{
    if (_gamepad.sdlHandle)
    {
        SDL_CloseGamepad(_gamepad.sdlHandle);
    }
}

void InputManager::Update()
{
    for (auto& key : _keyboard.keyPressed)
        key.second = false;
    for (auto& key : _keyboard.keyReleased)
        key.second = false;

    for (auto& button : _mouse.buttonPressed)
        button.second = false;
    for (auto& button : _mouse.buttonReleased)
        button.second = false;

    if(_gamepad.sdlHandle)
    {
        for (auto& button : _gamepad.buttonPressed)
            button.second = false;
        for (auto& button : _gamepad.buttonReleased)
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
            _keyboard.keyPressed[key] = true;
            _keyboard.keyHeld[key] = true;
        }
        break;
    case SDL_EVENT_KEY_UP:
    {
        KeyboardCode key = static_cast<KeyboardCode>(event.key.key);
        _keyboard.keyHeld[key] = false;
        _keyboard.keyReleased[key] = true;
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        _mouse.buttonPressed[button] = true;
        _mouse.buttonHeld[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        _mouse.buttonHeld[button] = false;
        _mouse.buttonReleased[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        _mouse.positionX += event.motion.xrel;
        _mouse.positionY += event.motion.yrel;
        break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    {
        GamepadButton button = static_cast<GamepadButton>(event.jbutton.button);
        _gamepad.buttonPressed[button] = true;
        _gamepad.buttonHeld[button] = true;
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
    {
        GamepadButton button = static_cast<GamepadButton>(event.jbutton.button);
        _gamepad.buttonHeld[button] = false;
        _gamepad.buttonReleased[button] = true;
        break;
    }
    default:
        break;
    }
}

bool InputManager::IsKeyPressed(KeyboardCode key) const
{
    return detail::unordered_map_get_or(_keyboard.keyPressed, key, false);
}

bool InputManager::IsKeyHeld(KeyboardCode key) const
{
    return detail::unordered_map_get_or(_keyboard.keyHeld, key, false);
}

bool InputManager::IsKeyReleased(KeyboardCode key) const
{
    return detail::unordered_map_get_or(_keyboard.keyReleased, key, false);
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    return detail::unordered_map_get_or(_mouse.buttonPressed, button, false);
}

bool InputManager::IsMouseButtonHeld(MouseButton button) const
{
    return detail::unordered_map_get_or(_mouse.buttonHeld, button, false);
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const
{
    return detail::unordered_map_get_or(_mouse.buttonReleased, button, false);
}

void InputManager::GetMousePosition(int& x, int& y) const
{
    x = static_cast<int>(_mouse.positionX);
    y = static_cast<int>(_mouse.positionY);
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button) const
{
    if (!_gamepad.sdlHandle)
    {
        bblog::warn("Trying to get gamepad input while no controller is available.");
        return false;
    }

    return detail::unordered_map_get_or(_gamepad.buttonPressed, button, false);
}

bool InputManager::IsGamepadButtonHeld(GamepadButton button) const
{
    if (!_gamepad.sdlHandle)
    {
        bblog::warn("Trying to get gamepad input while no controller is available.");
        return false;
    }

    return detail::unordered_map_get_or(_gamepad.buttonHeld, button, false);
}

bool InputManager::IsGamepadButtonReleased(GamepadButton button) const
{
    if (!_gamepad.sdlHandle)
    {
        bblog::warn("Trying to get gamepad input while no controller is available.");
        return false;
    }

    return detail::unordered_map_get_or(_gamepad.buttonReleased, button, false);
}

float InputManager::GetGamepadAxis(GamepadAxis axis) const
{
    if (!_gamepad.sdlHandle)
    {
        bblog::warn("Trying to get gamepad input while no controller is available.");
        return 0.0f;
    }

    SDL_GamepadAxis sdlAxis = static_cast<SDL_GamepadAxis>(axis);
    return SDL_GetGamepadAxis(_gamepad.sdlHandle, sdlAxis);
}