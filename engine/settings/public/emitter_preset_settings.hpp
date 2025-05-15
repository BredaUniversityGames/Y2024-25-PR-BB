#pragma once

#include "log.hpp"
#include "serialization_helpers.hpp"

struct ParticleBurst
{
    VERSION(0);

    float startTime = 0.0f;
    uint32_t count = 0;
    uint32_t cycles = 0;
    float maxInterval = 0.0f;
    float currentInterval = 0.0f;
    bool loop = true;
};

struct EmitterPreset
{
    VERSION(0);

    glm::vec3 size = { 1.0f, 1.0f, 0.0f }; // size (2) + size velocity (1)
    float mass = 1.0f;
    glm::vec2 rotationVelocity = { 0.0f, 0.0f }; // angle (1) + angle velocity (1)
    float maxLife = 5.0f;
    float emitDelay = 1.0f;
    uint32_t count = 0;
    uint32_t materialIndex = 0;
    glm::vec3 spawnRandomness = { 0.0f, 0.0f, 0.0f };
    uint32_t flags = 0;
    glm::vec3 velocityRandomness = { 0.0f, 0.0f, 0.0f };
    glm::vec3 startingVelocity = { 1.0f, 5.0f, 1.0f };
    glm::ivec2 spriteDimensions = { 1.0f, 1.0f };
    uint32_t frameCount = 1;
    float frameRate = 0.0f;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // color (3) + color multiplier (1)
    std::list<ParticleBurst> bursts = {};
    std::string name = "Emitter Preset";
    std::string imageName = "null";
};

struct EmitterPresetSettings
{
    VERSION(0);

    std::vector<EmitterPreset> emitterPresets;
};

VISITABLE_STRUCT(ParticleBurst, startTime, count, cycles, maxInterval, currentInterval, loop);
CLASS_SERIALIZE_VERSION(ParticleBurst);
CLASS_VERSION(ParticleBurst);

VISITABLE_STRUCT(EmitterPreset, size, mass, rotationVelocity, maxLife, emitDelay, count, materialIndex, spawnRandomness, flags, velocityRandomness, startingVelocity, spriteDimensions, frameCount, frameRate, color, bursts, name, imageName);
CLASS_SERIALIZE_VERSION(EmitterPreset);
CLASS_VERSION(EmitterPreset);

VISITABLE_STRUCT(EmitterPresetSettings, emitterPresets);
CLASS_SERIALIZE_VERSION(EmitterPresetSettings);
CLASS_VERSION(EmitterPresetSettings);