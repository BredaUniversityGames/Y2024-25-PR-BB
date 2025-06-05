#pragma once
#include "keys.hpp"
#include "mousebuttons.hpp"
#include <unordered_map>
#include <string>

std::unordered_map<KeyboardCode, std::string> KEYBOARD_KEY_NAMES =
{
    { KeyboardCode::eESCAPE, "Escape" },
    { KeyboardCode::eE, "E" },
    { KeyboardCode::eLSHIFT, "Left Shift" },
    { KeyboardCode::eR, "R" },
    { KeyboardCode::eSPACE, "Space" },
    { KeyboardCode::eW, "W" },
    { KeyboardCode::eS, "S" },
    { KeyboardCode::eA, "A" },
    { KeyboardCode::eD, "D" },
    { KeyboardCode::eF, "F" },
};

std::unordered_map<MouseButton, std::string> MOUSE_BUTTON_NAMES =
{
    { MouseButton::eBUTTON_LEFT, "Left Mouse" },
    { MouseButton::eBUTTON_MIDDLE, "Middle Mouse" },
    { MouseButton::eBUTTON_RIGHT, "Right Mouse" },
};
