#pragma once
#include <cstdint>

// Modified from SDL_gamepad.h

enum class GamepadButton : uint32_t
{
    eSOUTH, /* Bottom face button (e.g. Xbox A button) */
    eEAST, /* Right face button (e.g. Xbox B button) */
    eWEST, /* Left face button (e.g. Xbox X button) */
    eNORTH, /* Top face button (e.g. Xbox Y button) */
    eBACK,
    eGUIDE,
    eSTART,
    eLEFT_STICK,
    eRIGHT_STICK,
    eLEFT_SHOULDER,
    eRIGHT_SHOULDER,
    eDPAD_UP,
    eDPAD_DOWN,
    eDPAD_LEFT,
    eDPAD_RIGHT,
    eMISC1, /* Additional button (e.g. Xbox Series X share button, PS5 microphone button, Nintendo Switch Pro capture button, Amazon Luna microphone button, Google Stadia capture button) */
    eRIGHT_PADDLE1, /* Upper or primary paddle, under your right hand (e.g. Xbox Elite paddle P1) */
    eLEFT_PADDLE1, /* Upper or primary paddle, under your left hand (e.g. Xbox Elite paddle P3) */
    eRIGHT_PADDLE2, /* Lower or secondary paddle, under your right hand (e.g. Xbox Elite paddle P2) */
    eLEFT_PADDLE2, /* Lower or secondary paddle, under your left hand (e.g. Xbox Elite paddle P4) */
    eTOUCHPAD, /* PS4/PS5 touchpad button */
    eMISC2, /* Additional button */
    eMISC3, /* Additional button */
    eMISC4, /* Additional button */
    eMISC5, /* Additional button */
    eMISC6, /* Additional button */
    eLEFT_TRIGGER,
    eRIGHT_TRIGGER,
};

enum class GamepadAxis : uint32_t
{
    eLEFTX,
    eLEFTY,
    eRIGHTX,
    eRIGHTY,
    eLEFT_TRIGGER,
    eRIGHT_TRIGGER,
};

enum class GamepadAnalog : uint32_t
{
    eAXIS_LEFT,
    eAXIS_RIGHT,
    eDPAD,
};

enum class GamepadType : uint32_t
{
    eUnknown, // Catch-all for unrecognized devices
    eSteamController, // Valve's Steam Controller
    eXBox360Controller, // Microsoft's XBox 360 Controller
    eXBoxOneController, // Microsoft's XBox One Controller
    eGenericXInput, // Any generic 3rd-party XInput device
    ePS4Controller, // Sony's PlayStation 4 Controller
    ePS5Controller, // Sony's PlayStation 5 Controller
    eSwitchProController, // Nintendo's Switch Pro Controller
    eMobileTouch, // Steam Link App's Mobile Touch Controller
    ePS3Controller, // Sony's PlayStation 3 Controller
};
