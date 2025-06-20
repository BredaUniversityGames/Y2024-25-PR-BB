#pragma once

#include "enum_utils.hpp"
#include <cstdint>
#include <glm/glm.hpp>

// temporary values for testing/progress
static constexpr uint32_t MAX_EMITTERS = 512;
static constexpr int32_t MAX_PARTICLES = 1024 * 64;

enum class ParticleRenderFlagBits : uint8_t
{
    eUnlit = 1 << 0,
    eNoShadow = 1 << 1,
    eFrameBlend = 1 << 2,
    eLockY = 1 << 3, // lock y-axis when rotating to camera
    eIsLocal = 1 << 4, // particle follows emitter position
};
GENERATE_ENUM_FLAG_OPERATORS(ParticleRenderFlagBits)

// Structs in line with shaders
struct alignas(16) Emitter
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    uint32_t count = 0;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    float mass = 0.0f;
    glm::vec2 rotationVelocity = { 0.0f, 0.0f };
    float maxLife = 1.0f;
    float randomValue = 0.0f;
    glm::vec3 size = { 1.0f, 1.0f, 0.0f };
    uint32_t materialIndex = 0;
    glm::vec3 spawnRandomness = { 1.0f, 1.0f, 1.0f };
    uint8_t flags = 0;
    glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec3 velocityRandomness = { 0.0f, 0.0f, 0.0f };
    float frameRate = 0.0f;
    glm::ivec2 maxFrames = { 1, 1 };
    uint32_t frameCount = 1;
    uint32_t id = 0;
};

struct alignas(16) LocalEmitter
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    float alpha = 1.0;
    uint32_t id = 0;
};

struct alignas(16) Particle
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    float mass = 0.0f;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    float maxLife = 5.0f;
    glm::vec2 rotationVelocity = { 0.0f, 0.0f };
    float life = 5.0f;
    uint32_t materialIndex = 0;
    glm::vec3 size = { 1.0f, 1.0f, 0.0f };
    uint8_t flags = 0;
    glm::vec3 color = { 1.0f, 1.0f, 1.0f };
    float frameRate = 0.0f;
    glm::vec3 velocityRandomness = { 0.0f, 0.0f, 0.0f };
    uint32_t frameCount = 1;
    glm::ivec2 maxFrames = { 1, 1 };
    glm::vec2 textureMultiplier = { 1.0f, 1.0f };
    uint32_t emitterId = 0;
};

struct alignas(16) ParticleCounters
{
    uint32_t aliveCount = 0;
    uint32_t deadCount = MAX_PARTICLES;
    uint32_t aliveCountAfterSimulation = 0;
};

struct alignas(16) ParticleInstance
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    uint32_t materialIndex = 0;
    glm::vec2 size = { 1.0f, 1.0f };
    float angle = 0.0f;
    uint8_t flags = 0;
    glm::vec3 color = { 1.0f, 1.0f, 1.0f };
    float frameBlend = 0.0f;
    glm::ivec2 frameOffsetCurrent = { 0, 0 };
    glm::ivec2 frameOffsetNext = { 0, 0 };
    glm::vec2 textureMultiplier = { 1.0f, 1.0f };
    float alpha = 1.0;
};