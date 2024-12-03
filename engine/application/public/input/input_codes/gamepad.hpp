#pragma once

// Modified from SDL_gamepad.h

enum class GamepadButton : uint32_t
{
    eGAMEPAD_BUTTON_SOUTH, /* Bottom face button (e.g. Xbox A button) */
    eGAMEPAD_BUTTON_EAST, /* Right face button (e.g. Xbox B button) */
    eGAMEPAD_BUTTON_WEST, /* Left face button (e.g. Xbox X button) */
    eGAMEPAD_BUTTON_NORTH, /* Top face button (e.g. Xbox Y button) */
    eGAMEPAD_BUTTON_BACK,
    eGAMEPAD_BUTTON_GUIDE,
    eGAMEPAD_BUTTON_START,
    eGAMEPAD_BUTTON_LEFT_STICK,
    eGAMEPAD_BUTTON_RIGHT_STICK,
    eGAMEPAD_BUTTON_LEFT_SHOULDER,
    eGAMEPAD_BUTTON_RIGHT_SHOULDER,
    eGAMEPAD_BUTTON_DPAD_UP,
    eGAMEPAD_BUTTON_DPAD_DOWN,
    eGAMEPAD_BUTTON_DPAD_LEFT,
    eGAMEPAD_BUTTON_DPAD_RIGHT,
    eGAMEPAD_BUTTON_MISC1, /* Additional button (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button) */
    eGAMEPAD_BUTTON_RIGHT_PADDLE1, /* Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1) */
    eGAMEPAD_BUTTON_LEFT_PADDLE1, /* Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3) */
    eGAMEPAD_BUTTON_RIGHT_PADDLE2, /* Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2) */
    eGAMEPAD_BUTTON_LEFT_PADDLE2, /* Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4) */
    eGAMEPAD_BUTTON_TOUCHPAD, /* PS4/PS5 touchpad button */
    eGAMEPAD_BUTTON_MISC2, /* Additional button */
    eGAMEPAD_BUTTON_MISC3, /* Additional button */
    eGAMEPAD_BUTTON_MISC4, /* Additional button */
    eGAMEPAD_BUTTON_MISC5, /* Additional button */
    eGAMEPAD_BUTTON_MISC6, /* Additional button */
};

enum class GamepadAxis : uint32_t
{
    eGAMEPAD_AXIS_LEFTX,
    eGAMEPAD_AXIS_LEFTY,
    eGAMEPAD_AXIS_RIGHTX,
    eGAMEPAD_AXIS_RIGHTY,
    eGAMEPAD_AXIS_LEFT_TRIGGER,
    eGAMEPAD_AXIS_RIGHT_TRIGGER,
    eGAMEPAD_AXIS_COUNT
};