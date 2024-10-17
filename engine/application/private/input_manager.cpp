#include "input_manager.hpp"
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
    SDL_GetMouseState(&_mouseX, &_mouseY);
}

void InputManager::Update()
{
    // Reset key and mouse button states
    for (auto& key : _keyPressed)
        key.second = false;
    for (auto& key : _keyReleased)
        key.second = false;
    for (auto& button : _mouseButtonPressed)
        button.second = false;
    for (auto& button : _mouseButtonReleased)
        button.second = false;
}

void InputManager::UpdateEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
        if (event.key.repeat == 0)
        { // Only process on first keydown, not when holding
            KeyboardCode key = static_cast<KeyboardCode>(event.key.key);
            _keyPressed[key] = true;
            _keyHeld[key] = true;
        }
        break;
    case SDL_EVENT_KEY_UP:
    {
        KeyboardCode key = static_cast<KeyboardCode>(event.key.key);
        _keyHeld[key] = false;
        _keyReleased[key] = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        _mouseButtonPressed[button] = true;
        _mouseButtonHeld[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        _mouseButtonHeld[button] = false;
        _mouseButtonReleased[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        // Handle mouse motion event if necessary
        _mouseX += event.motion.xrel;
        _mouseY += event.motion.yrel;

        break;
    case SDL_EVENT_QUIT:
        // TODO: handle this
        // Handle quit event if necessary
        break;
    default:
        break;
    }
}

bool InputManager::IsKeyPressed(KeyboardCode key) const
{
    return detail::unordered_map_get_or(_keyPressed, key, false);
}

bool InputManager::IsKeyHeld(KeyboardCode key) const
{
    return detail::unordered_map_get_or(_keyHeld, key, false);
}

bool InputManager::IsKeyReleased(KeyboardCode key) const
{
    return detail::unordered_map_get_or(_keyReleased, key, false);
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    return detail::unordered_map_get_or(_mouseButtonPressed, button, false);
}

bool InputManager::IsMouseButtonHeld(MouseButton button) const
{
    return detail::unordered_map_get_or(_mouseButtonHeld, button, false);
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const
{
    return detail::unordered_map_get_or(_mouseButtonReleased, button, false);
}

void InputManager::GetMousePosition(int& x, int& y) const
{
    float fx {}, fy {};
    SDL_GetMouseState(&fx, &fy);
    x = static_cast<int>(_mouseX); // fx;
    y = static_cast<int>(_mouseY); // fy;
}
