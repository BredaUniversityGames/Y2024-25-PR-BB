#include "input_manager.hpp"
#include <algorithm>

InputManager::InputManager()
{
    SDL_GetMouseState(&mouseX, &mouseY);
}

InputManager::~InputManager()
{

}

void InputManager::Update()
{
    // Reset key and mouse button states
    for (auto& key : keyPressed) key.second = false;
    for (auto& key : keyReleased) key.second = false;
    for (auto& button : mouseButtonPressed) button.second = false;
    for (auto& button : mouseButtonReleased) button.second = false;
}

void InputManager::UpdateEvent(SDL_Event event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
        if (event.key.repeat == 0) { // Only process on first keydown, not when holding
            Key key = static_cast<Key>(event.key.scancode);
            keyPressed[key] = true;
            keyHeld[key] = true;
        }
        break;
    case SDL_EVENT_KEY_UP:
    {
        Key key = static_cast<Key>(event.key.scancode);
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
        // Handle quit event if necessary
        break;
    default:
        break;
    }
}

bool InputManager::IsKeyPressed(Key key) const
{
    return keyPressed[key];
}

bool InputManager::IsKeyHeld(Key key) const
{
    return keyHeld[key];
}

bool InputManager::IsKeyReleased(Key key) const
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
    x = fx;//fx;
    y = fy;//fy;
}
