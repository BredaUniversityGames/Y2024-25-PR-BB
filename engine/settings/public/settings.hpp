#pragma once

#include <glm/vec3.hpp>

#include "serialization_helpers.hpp"

struct Settings
{
    glm::vec3 fogColor { 0.5, 0.6, 0.7 };
    float fogDensity { 0.2f };
    float fogHeight { 0.3f };

    template <class Archive>
    void serialize(Archive& archive)
    {
        archive(fogColor, fogDensity, fogHeight);
    }
};

class SettingsStore
{
public:
    static SettingsStore& Instance();

    void Write();

    Settings settings;

private:
    SettingsStore();
};
