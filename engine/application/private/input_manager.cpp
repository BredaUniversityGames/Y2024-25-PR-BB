#include "input_manager.hpp"
#include "log.hpp"

#include <SDL3/SDL.h>

InputManager::InputManager()
{
    SDL_GetMouseState(&mouseX, &mouseY);
}

void InputManager::Update()
{
    // Reset key and mouse button states
    for (auto& key : keyPressed)
        key.second = false;
    for (auto& key : keyReleased)
        key.second = false;
    for (auto& button : mouseButtonPressed)
        button.second = false;
    for (auto& button : mouseButtonReleased)
        button.second = false;
}

void InputManager::UpdateEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
        if (event.key.repeat == 0)
        { // Only process on first keydown, not when holding
            KeyCode key = static_cast<KeyCode>(event.key.key);
            keyPressed[key] = true;
            keyHeld[key] = true;
        }
        break;
    case SDL_EVENT_KEY_UP:
    {
        KeyCode key = static_cast<KeyCode>(event.key.key);
        keyHeld[key] = false;
        keyReleased[key] = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        mouseButtonPressed[button] = true;
        mouseButtonHeld[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        MouseButton button = static_cast<MouseButton>(event.button.button);
        mouseButtonHeld[button] = false;
        mouseButtonReleased[button] = true;
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        // Handle mouse motion event if necessary
        mouseX += event.motion.xrel;
        mouseY += event.motion.yrel;

        break;
    case SDL_EVENT_QUIT:
        // TODO: handle this
        // Handle quit event if necessary
        break;
    default:
        break;
    }
}

bool InputManager::IsKeyPressed(KeyCode key) const
{
    return keyPressed[key];
}

bool InputManager::IsKeyHeld(KeyCode key) const
{
    return keyHeld[key];
}

bool InputManager::IsKeyReleased(KeyCode key) const
{
    return keyReleased[key];
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    return mouseButtonPressed[button];
}

bool InputManager::IsMouseButtonHeld(MouseButton button) const
{
    return mouseButtonHeld[button];
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const
{
    return mouseButtonReleased[button];
}

void InputManager::GetMousePosition(int& x, int& y) const
{
    float fx, fy;
    SDL_GetMouseState(&fx, &fy);
    x = mouseX; // fx;
    y = mouseY; // fy;
}
