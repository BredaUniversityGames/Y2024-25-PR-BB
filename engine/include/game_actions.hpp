#pragma once
#include "input/action_manager.hpp"

const GameActions GAME_ACTIONS {
    {
        // Action Set
        .name = "Shooter",
        .digitalActions = {
            {
                .name = "Menu",
                .type = DigitalActionType::ePressed,
                .inputs = {
                    GamepadButton::eSTART,
                    GamepadButton::eBACK,
                },
            },
            {
                .name = "Slide",
                .type = DigitalActionType::ePressed,
                .inputs = {
                    GamepadButton::eLEFT_SHOULDER,
                    GamepadButton::eEAST,
                },
            },
            {
                .name = "Dash",
                .type = DigitalActionType::ePressed,
                .inputs = {
                    GamepadButton::eLEFT_TRIGGER,
                },
            },
            {
                .name = "Grenade",
                .type = DigitalActionType::ePressed,
                .inputs = { GamepadButton::eRIGHT_SHOULDER },
            },
            {
                .name = "Shoot",
                .type = DigitalActionType::ePressed,
                .inputs = { GamepadButton::eRIGHT_TRIGGER, MouseButton::eBUTTON_LEFT },
            },
            {
                .name = "Ultimate",
                .type = DigitalActionType::ePressed,
                .inputs = {
                    GamepadButton::eNORTH,
                },
            },
            {
                .name = "Reload",
                .type = DigitalActionType::ePressed,
                .inputs = { GamepadButton::eWEST, KeyboardCode::eR },
            },
            {
                .name = "Jump",
                .type = DigitalActionType::ePressed,
                .inputs = { GamepadButton::eSOUTH, KeyboardCode::eSPACE },
            },
            {
                .name = "Melee",
                .type = DigitalActionType::ePressed,
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
