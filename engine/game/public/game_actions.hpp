#pragma once
#include "input/action_manager.hpp"

const GameActions GAME_ACTIONS {
    {
        // Action Set
        .name = "Shooter",
        .digitalActions = {
            {
                .name = "Pause",
                .inputs = {
                    GamepadButton::eSTART,
                    GamepadButton::eBACK,
                    KeyboardCode::eESCAPE },
            },
            {
                .name = "Interact",
                .inputs = {
                    GamepadButton::eNORTH,
                    KeyboardCode::eE,
                },
            },
            {
                .name = "Dash",
                .inputs = {
                    GamepadButton::eEAST,
                    GamepadButton::eLEFT_SHOULDER,
                    KeyboardCode::eLSHIFT,
                },
            },
            {
                .name = "Shoot",
                .inputs = { GamepadButton::eRIGHT_TRIGGER, MouseButton::eBUTTON_LEFT },
            },
            {
                .name = "ShootSecondary",
                .inputs = { GamepadButton::eLEFT_TRIGGER, MouseButton::eBUTTON_RIGHT },
            },
            {
                .name = "Reload",
                .inputs = { GamepadButton::eWEST, KeyboardCode::eR },
            },
            {
                .name = "Jump",
                .inputs = { GamepadButton::eSOUTH, KeyboardCode::eSPACE },
            },
        },
        .analogActions = {
            {
                .name = "Move",
                .inputs = {
                    GamepadAnalog::eAXIS_LEFT,
                    GamepadAnalog::eDPAD,
                    KeyboardAnalog {
                        .up = KeyboardCode::eW,
                        .down = KeyboardCode::eS,
                        .left = KeyboardCode::eA,
                        .right = KeyboardCode::eD,
                    },
                },
            },
            {
                .name = "Look",
                .inputs = {
                    GamepadAnalog::eAXIS_RIGHT,
                    MouseAnalog {},
                },
            },
        },
    },
    {
        .name = "UserInterface",
        .digitalActions = {
            {
                .name = "Interact",
                .inputs = {
                    { GamepadButton::eSOUTH, KeyboardCode::eE },
                },
            },
            {
                .name = "Unpause",
                .inputs = { GamepadButton::eSTART, GamepadButton::eBACK, KeyboardCode::eESCAPE },
            },
            {
                .name = "Back",
                .inputs = {
                    GamepadButton::eEAST,
                },
            },
        },
        .analogActions = {
            {
                .name = "Navigate",
                .inputs = {
                    GamepadAnalog::eAXIS_LEFT,
                    GamepadAnalog::eDPAD,
                    MouseAnalog {},
                },
            },
        },
    },
};
