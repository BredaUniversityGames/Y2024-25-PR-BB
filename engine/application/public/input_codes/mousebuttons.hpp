#pragma once
#include <cstdint>

// Modified from SDL_mouse.h

enum class MouseButton : uint32_t
{
    eBUTTON_LEFT = 1,
    eBUTTON_MIDDLE = 2,
    eBUTTON_RIGHT = 3,
    eBUTTON_X1 = 4,
    eBUTTON_X2 = 5
};