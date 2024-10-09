#pragma once

// temporary values for testing/progress
static constexpr uint32_t MAX_EMITTERS = 32;
static constexpr int32_t MAX_PARTICLES = 1024;

// Structs in line with shaders
struct alignas(16) Emitter
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    uint32_t count = 0;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    float mass = 0.0f;
    glm::vec3 rotationVelocity = { 0.0f, 0.0f, 0.0f };
    float maxLife = 1.0f;
    // TODO: image/color
};

struct alignas(16) Particle
{
    glm::vec3 position = { 0.0f, 0.0f, 0.0f };
    float mass = 0.0f;
    glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };
    float maxLife = 5.0f;
    glm::vec3 rotationVelocity = { 0.0f, 0.0f, 0.0f };
    float life = 5.0f;
    // TODO: image/color
};

struct alignas(16) ParticleCounters
{
    uint32_t aliveCount;
    int32_t deadCount;
    uint32_t aliveCount_afterSimulation;
    uint32_t culledCount;
};


enum class ParticleType
{
    eBillboard = 0,
    eRibbon,
    eNone // TODO: is this required? good practice?
};