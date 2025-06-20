#pragma once

#include "serialization_helpers.hpp"

struct ParticleBurst
{
    float startTime = 0.0f;
    uint32_t count = 0;
    uint32_t cycles = 0;
    float maxInterval = 0.0f;
    float currentInterval = 0.0f;
    bool loop = true;
};

struct EmitterPreset
{
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
    std::string imageName = "null";
};

struct EmitterPresetSettings
{
    std::unordered_map<std::string, EmitterPreset> emitterPresets;
};

VISITABLE_STRUCT(ParticleBurst, startTime, count, cycles, maxInterval, currentInterval, loop);
CLASS_SERIALIZE_VERSION(ParticleBurst, 0);

VISITABLE_STRUCT(EmitterPreset, size, mass, rotationVelocity, maxLife, emitDelay, count, materialIndex, spawnRandomness, flags, velocityRandomness, startingVelocity, spriteDimensions, frameCount, frameRate, color, bursts, imageName);
CLASS_SERIALIZE_VERSION(EmitterPreset, 0);

VISITABLE_STRUCT(EmitterPresetSettings, emitterPresets);
CLASS_SERIALIZE_VERSION(EmitterPresetSettings, 0);