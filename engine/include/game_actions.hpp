#pragma once
#include "input/action_manager.hpp"

const GameActions GAME_ACTIONS {
    {
        // Action Set
        .name = "Shooter",
        .digitalActions = {
            {
                .name = "Menu",
                .inputs = {
                    GamepadButton::eSTART,
                    GamepadButton::eBACK,
                },
            },
            {
                .name = "Slide",
                .inputs = {
                    GamepadButton::eLEFT_SHOULDER,
                    GamepadButton::eEAST,
                },
            },
            {
                .name = "Dash",
                .inputs = {
                    GamepadButton::eLEFT_TRIGGER,
                },
            },
            {
                .name = "Grenade",
                .inputs = { GamepadButton::eRIGHT_SHOULDER },
            },
            {
                .name = "Shoot",
                .inputs = { GamepadButton::eRIGHT_TRIGGER, MouseButton::eBUTTON_LEFT },
            },
            {
                .name = "Ultimate",
                .inputs = {
                    GamepadButton::eNORTH,
                },
            },
            {
                .name = "Reload",
                .inputs = { GamepadButton::eWEST, KeyboardCode::eR },
            },
            {
                .name = "Jump",
                .inputs = { GamepadButton::eSOUTH, KeyboardCode::eSPACE },
            },
            {
                .name = "Melee",
                .inputs = {
                    GamepadButton::eRIGHT_STICK,
                },
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
                },
            },
        },
    },
};
