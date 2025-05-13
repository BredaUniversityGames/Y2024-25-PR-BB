#pragma once
#include "serialization_helpers.hpp"

struct GameSettings
{
    static GameSettings FromFile(const std::string& path);
    void SaveToFile(const std::string& path) const;

    bool framerateCounter = false;
    float aimSensitivity = 0.5f;
    float gammaSlider = 0.5f;
    bool vsync = false;
    bool aimAssist = false;
    float masterVolume = 0.5f;
    float sfxVolume = 1.0f;
    float musicVolume = 1.0f;
};

constexpr const char* GAME_SETTINGS_FILE = "game_settings.json";

VISITABLE_STRUCT(GameSettings, framerateCounter, aimSensitivity, gammaSlider, vsync, aimAssist, masterVolume, sfxVolume, musicVolume);
CLASS_SERIALIZE_VERSION(GameSettings, 1);