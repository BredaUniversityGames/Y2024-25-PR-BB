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
                .inputs = {
                    GamepadButton::eRIGHT_SHOULDER,
                },
            },
            {
                .name = "Shoot",
                .type = DigitalActionType::eHold,
                .inputs = {
                    GamepadButton::eRIGHT_TRIGGER,
                },
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
                .inputs = {
                    GamepadButton::eWEST,
                },
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
