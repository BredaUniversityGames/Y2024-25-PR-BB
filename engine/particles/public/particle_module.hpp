#pragma once

#include "resource_manager.hpp"

#include "common.hpp"
#include "entt/entity/entity.hpp"
#include "module_interface.hpp"
#include "particle_util.hpp"

#include <memory>

class GraphicsContext;
struct GPUImage;
class ECSModule;
class PhysicsModule;

// TODO: make bitflag utility base class if we want to use it in more places
enum class SpawnEmitterFlagBits : uint8_t
{
    eEmitOnce = 1 << 0,
    eIsActive = 1 << 1,
    eSetCustomPosition = 1 << 2,
    eSetCustomVelocity = 1 << 3
};
GENERATE_ENUM_FLAG_OPERATORS(SpawnEmitterFlagBits)

enum class EmitterPresetID : uint8_t
{
    eTest = 0,
    eFlame,
    eDust,
    eImpact,
    eRay,
    eNone
};

class ParticleModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
    void Tick(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Particle Module"; }

public:
    ParticleModule() = default;
    ~ParticleModule() override = default;

    void LoadEmitterPresets();
    void SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, SpawnEmitterFlagBits spawnEmitterFlagBits, glm::vec3 position = { 0.0f, 0.0f, 0.0f }, glm::vec3 velocity = { 5.0f, 5.0f, 5.0f });
    void SpawnEmitter(entt::entity entity, int32_t emitterPresetID, SpawnEmitterFlagBits spawnEmitterFlagBits, glm::vec3 position = { 0.0f, 0.0f, 0.0f }, glm::vec3 velocity = { 5.0f, 5.0f, 5.0f });

private:
    std::shared_ptr<GraphicsContext> _context;
    ECSModule* _ecs = nullptr;
    PhysicsModule* _physics = nullptr;

    struct EmitterPreset
    {
        glm::vec3 size = { 1.0f, 1.0f, 0.0f }; // 2d size + size velocity
        float mass = 1.0f;
        glm::vec2 rotationVelocity = { 0.0f, 0.0f }; // angle + angle velocity
        float maxLife = 5.0f;
        float emitDelay = 1.0f;
        uint32_t count = 0;
        uint32_t materialIndex = 0;
        glm::vec3 spawnRandomness = { 1.0f, 1.0f, 1.0f }; // y is inused as of now for randomness
        uint32_t flags = 0;
        glm::vec3 velocityRandomness = { 0.0f, 0.0f, 0.0f };
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // color + color multiplier
        std::string name = "Emitter Preset";
    };

    ResourceHandle<GPUImage>& GetEmitterImage(std::string fileName);
    void SetEmitterPresetImage(EmitterPreset& preset, ResourceHandle<GPUImage> image);

    std::vector<EmitterPreset> _emitterPresets;
    std::unordered_map<std::string, ResourceHandle<GPUImage>> _emitterImages;

    friend class ParticleEditor;
};