#pragma once

#include <SDL3/SDL.h>
#include <unordered_map>

class InputManager
{
public:
    enum class Key
    {
        W = SDL_SCANCODE_W,
        A = SDL_SCANCODE_A,
        S = SDL_SCANCODE_S,
        D = SDL_SCANCODE_D,
        Space = SDL_SCANCODE_SPACE,
        Escape = SDL_SCANCODE_ESCAPE,
        F1 = SDL_SCANCODE_F1,
        F2 = SDL_SCANCODE_F2,
        H = SDL_SCANCODE_H,
        P = SDL_SCANCODE_P,
        // Add other keys as needed
    };

    enum class MouseButton
    {
        Left = SDL_BUTTON_LEFT,
        Right = SDL_BUTTON_RIGHT,
        Middle = SDL_BUTTON_MIDDLE,
        // Add other mouse buttons as needed
    };

    InputManager();
    ~InputManager();

    void Update();
    void UpdateEvent(SDL_Event event);

    bool IsKeyPressed(Key key) const;
    bool IsKeyHeld(Key key) const;
    bool IsKeyReleased(Key key) const;

    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonHeld(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;

    void GetMousePosition(int& x, int& y) const;

private:
    mutable std::unordered_map<Key, bool> keyPressed;
    mutable std::unordered_map<Key, bool> keyHeld;
    mutable std::unordered_map<Key, bool> keyReleased;

    mutable std::unordered_map<MouseButton, bool> mouseButtonPressed;
    mutable std::unordered_map<MouseButton, bool> mouseButtonHeld;
    mutable std::unordered_map<MouseButton, bool> mouseButtonReleased;

    float mouseX, mouseY;
};
