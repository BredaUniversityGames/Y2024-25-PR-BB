#pragma once

#include "input_codes/keys.hpp"
#include "input_codes/mousebuttons.hpp"
#include <unordered_map>

union SDL_Event;

class InputManager
{
public:
    InputManager();
    ~InputManager() = default;

    void Update();
    void UpdateEvent(const SDL_Event& event);

    bool IsKeyPressed(KeyboardCode key) const;
    bool IsKeyHeld(KeyboardCode key) const;
    bool IsKeyReleased(KeyboardCode key) const;

    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonHeld(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;

    void GetMousePosition(int& x, int& y) const;

private:
    std::unordered_map<KeyboardCode, bool> _keyPressed;
    std::unordered_map<KeyboardCode, bool> _keyHeld;
    std::unordered_map<KeyboardCode, bool> _keyReleased;

    std::unordered_map<MouseButton, bool> _mouseButtonPressed;
    std::unordered_map<MouseButton, bool> _mouseButtonHeld;
    std::unordered_map<MouseButton, bool> _mouseButtonReleased;

    float _mouseX {}, _mouseY {};
};