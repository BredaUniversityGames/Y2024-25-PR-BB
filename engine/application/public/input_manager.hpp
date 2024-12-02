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
    struct Mouse
    {
        std::unordered_map<MouseButton, bool> buttonPressed{};
        std::unordered_map<MouseButton, bool> buttonHeld{};
        std::unordered_map<MouseButton, bool> buttonReleased{};
        float positionX {}, positionY {};
    } _mouse{};

    struct Keyboard
    {
        std::unordered_map<KeyboardCode, bool> keyPressed{};
        std::unordered_map<KeyboardCode, bool> keyHeld{};
        std::unordered_map<KeyboardCode, bool> keyReleased{};
    } _keyboard;
};