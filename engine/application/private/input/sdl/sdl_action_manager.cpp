#include "input/sdl/sdl_action_manager.hpp"
#include "input/sdl/sdl_input_device_manager.hpp"
#include "log.hpp"
#include <algorithm>
#include <magic_enum.hpp>

SDLActionManager::SDLActionManager(const SDLInputDeviceManager& sdlInputDeviceManager)
    : ActionManager(sdlInputDeviceManager)
    , _sdlInputDeviceManager(sdlInputDeviceManager)
{
}

DigitalActionType SDLActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadButton button) const
{
    DigitalActionType result {};

    if (_sdlInputDeviceManager.IsGamepadButtonPressed(button))
    {
        result = result | DigitalActionType::ePressed;
    }
    if (_sdlInputDeviceManager.IsGamepadButtonHeld(button))
    {
        result = result | DigitalActionType::eHeld;
    }
    if (_sdlInputDeviceManager.IsGamepadButtonReleased(button))
    {
        result = result | DigitalActionType::eReleased;
    }

    return result;
}

glm::vec2 SDLActionManager::CheckInput(MAYBE_UNUSED std::string_view actionName, GamepadAnalog gamepadAnalog) const
{
    glm::vec2 result = { 0.0f, 0.0f };

    if (!_inputDeviceManager.IsGamepadAvailable())
    {
        return result;
    }

    switch (gamepadAnalog)
    {
    // Steam Input allows to use DPAD as analog input, so we do the same for SDL input
    case GamepadAnalog::eDPAD:
    {
        result.x = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_LEFT)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_RIGHT));
        result.y = -static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_DOWN)) + static_cast<float>(_sdlInputDeviceManager.IsGamepadButtonHeld(GamepadButton::eDPAD_UP));
        break;
    }

    case GamepadAnalog::eAXIS_LEFT:
    case GamepadAnalog::eAXIS_RIGHT:
    {
        static const std::unordered_map<GamepadAnalog, std::pair<GamepadAxis, GamepadAxis>> GAMEPAD_ANALOG_AXIS_MAPPING {
            { GamepadAnalog::eAXIS_LEFT, { GamepadAxis::eLEFTX, GamepadAxis::eLEFTY } },
            { GamepadAnalog::eAXIS_RIGHT, { GamepadAxis::eRIGHTX, GamepadAxis::eRIGHTY } },
        };

        const std::pair<GamepadAxis, GamepadAxis> axes = GAMEPAD_ANALOG_AXIS_MAPPING.at(gamepadAnalog);
        result.x = _sdlInputDeviceManager.GetGamepadAxis(axes.first);
        result.y = _sdlInputDeviceManager.GetGamepadAxis(axes.second);
        break;
    }

    default:
    {
        bblog::error("[Input] Unsupported analog input \"{}\" for action: \"{}\"", magic_enum::enum_name(gamepadAnalog), actionName);
        break;
    }
    }

    return result;
}

std::vector<std::string> SDLActionManager::GetDigitalActionGamepadGlyphImagePaths(std::string_view actionName) const
{
    if (!_sdlInputDeviceManager.IsGamepadAvailable())
    {
        return {};
    }

    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return {};
    }

    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& digitalActions = actionSet.digitalActions;

    auto actionItr = std::find_if(digitalActions.begin(), digitalActions.end(),
        [actionName](const DigitalAction& action)
        { return action.name == actionName; });
    if (actionItr == actionSet.digitalActions.end())
    {
        bblog::error("[Input] Failed to find digital action: \"{}\"", actionName);
        return {};
    }

    GamepadType gamepadType = _sdlInputDeviceManager.GetGamepadType();
    auto customGlyphs = _gamepadGlyphs.find(gamepadType);
    if (customGlyphs == _gamepadGlyphs.end())
    {
        return {};
    }

    std::vector<std::string> glyphPaths {};

    for (const auto& inputBinding : actionItr->inputs)
    {
        if (!std::holds_alternative<GamepadButton>(inputBinding))
        {
            continue;
        }

        GamepadButton button = std::get<GamepadButton>(inputBinding);
        auto digitalGlyph = customGlyphs->second.digitals.find(button);
        if (digitalGlyph == customGlyphs->second.digitals.end())
        {
            glyphPaths.push_back(digitalGlyph->second);
        }
    }

    return glyphPaths;
}

std::vector<std::string> SDLActionManager::GetAnalogActionGamepadGlyphImagePaths(std::string_view actionName) const
{
    if (!_sdlInputDeviceManager.IsGamepadAvailable())
    {
        return {};
    }

    if (_gameActions.empty())
    {
        bblog::error("[Input] No game actions are set while trying to get action: \"{}\"", actionName);
        return {};
    }

    const ActionSet& actionSet = _gameActions[_activeActionSet];
    const auto& analogActions = actionSet.analogActions;

    auto actionItr = std::find_if(analogActions.begin(), analogActions.end(),
        [actionName](const AnalogAction& action)
        { return action.name == actionName; });
    if (actionItr == actionSet.analogActions.end())
    {
        bblog::error("[Input] Failed to find analog action: \"{}\"", actionName);
        return {};
    }

    GamepadType gamepadType = _sdlInputDeviceManager.GetGamepadType();
    auto customGlyphs = _gamepadGlyphs.find(gamepadType);
    if (customGlyphs == _gamepadGlyphs.end())
    {
        return {};
    }

    std::vector<std::string> glyphPaths {};

    for (const auto& inputBinding : actionItr->inputs)
    {
        if (!std::holds_alternative<GamepadAnalog>(inputBinding))
        {
            continue;
        }

        GamepadAnalog analog = std::get<GamepadAnalog>(inputBinding);
        auto analogGlyph = customGlyphs->second.analogs.find(analog);
        if (analogGlyph == customGlyphs->second.analogs.end())
        {
            glyphPaths.push_back(analogGlyph->second);
        }
    }

    return glyphPaths;
}
