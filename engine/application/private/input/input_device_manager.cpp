#include "input/input_device_manager.hpp"

#include "hashmap_utils.hpp"

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <imgui.h>

InputDeviceManager::InputDeviceManager()
{
    SDL_GetMouseState(&_mouse.positionX, &_mouse.positionY);
}

void InputDeviceManager::Update()
{
    for (auto& key : _keyboard.inputPressed)
        key.second = false;
    for (auto& key : _keyboard.inputReleased)
        key.second = false;

    for (auto& button : _mouse.inputPressed)
        button.second = false;
    for (auto& button : _mouse.inputReleased)
        button.second = false;
}

void InputDeviceManager::UpdateEvent(const SDL_Event& event)
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
    {
        _mouse.positionX += event.motion.xrel;
        _mouse.positionY += event.motion.yrel;

        break;
    }

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    {
        SDL_GetMouseState(&_mouse.positionX, &_mouse.positionY);
        break;
    }

    default:
        break;
    }
}

bool InputDeviceManager::IsKeyPressed(KeyboardCode key) const
{
    return !ImGui::GetIO().WantCaptureKeyboard && UnorderedMapGetOr(_keyboard.inputPressed, key, false);
}

bool InputDeviceManager::IsKeyHeld(KeyboardCode key) const
{
    return !ImGui::GetIO().WantCaptureKeyboard && UnorderedMapGetOr(_keyboard.inputHeld, key, false);
}

bool InputDeviceManager::IsKeyReleased(KeyboardCode key) const
{
    return !ImGui::GetIO().WantCaptureKeyboard && UnorderedMapGetOr(_keyboard.inputReleased, key, false);
}

bool InputDeviceManager::IsMouseButtonPressed(MouseButton button) const
{
    return !ImGui::GetIO().WantCaptureMouse && UnorderedMapGetOr(_mouse.inputPressed, button, false);
}

bool InputDeviceManager::IsMouseButtonHeld(MouseButton button) const
{
    return !ImGui::GetIO().WantCaptureMouse && UnorderedMapGetOr(_mouse.inputHeld, button, false);
}

bool InputDeviceManager::IsMouseButtonReleased(MouseButton button) const
{
    return !ImGui::GetIO().WantCaptureMouse && UnorderedMapGetOr(_mouse.inputReleased, button, false);
}

void InputDeviceManager::GetMousePosition(int32_t& x, int32_t& y) const
{
    x = static_cast<int32_t>(_mouse.positionX);
    y = static_cast<int32_t>(_mouse.positionY);
}

float InputDeviceManager::ClampGamepadAxisDeadzone(float input) const
{
    if (glm::abs(input) < INNER_GAMEPAD_DEADZONE)
    {
        return 0.0f;
    }

    if (glm::abs(input) > OUTER_GAMEPAD_DEADZONE)
    {
        return input < 0.0f ? MIN_GAMEPAD_AXIS : MAX_GAMEPAD_AXIS;
    }

    return input;
}