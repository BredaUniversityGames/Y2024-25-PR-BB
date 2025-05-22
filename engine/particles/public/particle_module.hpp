#pragma once

#include "data_store.hpp"
#include "emitter_preset_settings.hpp"

#include "resource_manager.hpp"

#include "common.hpp"
#include "emitter_component.hpp"
#include "entt/entity/entity.hpp"
#include "module_interface.hpp"
#include "particle_util.hpp"

#include <memory>

class GraphicsContext;
struct GPUImage;
class ECSModule;
class PhysicsModule;

enum class SpawnEmitterFlagBits : uint8_t
{
    eEmitOnce = 1 << 0,
    eIsActive = 1 << 1,
    eSetCustomPosition = 1 << 2,
    eSetCustomVelocity = 1 << 3,
};
GENERATE_ENUM_FLAG_OPERATORS(SpawnEmitterFlagBits)

enum class EmitterPresetID : uint8_t
{
    eFlame,
    eDust,
    eFeathers,
    eBullets,
    eBones,
    eImpact,
    eRay,
    eRayQuadDamage,
    eRayEyeStart,
    eRayEyeEnd,
    eStab,
    eShotgunShoot,
    eFireAnimated,
    eHealth,
    eWorms,
    eSoulSheet,
    eQuadStation,
    eDualGunStation,
    eNone
};

class ParticleModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
    void Tick(MAYBE_UNUSED Engine& engine) override;
    std::string_view GetName() override { return "Particle Module"; }

public:
    ParticleModule()
        : _emitterPresets("game/config/emitter_presets.json") {};
    ~ParticleModule() override = default;

    void LoadEmitterPresets();
    void SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, SpawnEmitterFlagBits spawnEmitterFlagBits, glm::vec3 position = { 0.0f, 0.0f, 0.0f }, glm::vec3 velocity = { 5.0f, 5.0f, 5.0f });
    void SpawnEmitter(entt::entity entity, int32_t emitterPresetID, SpawnEmitterFlagBits spawnEmitterFlagBits, glm::vec3 position = { 0.0f, 0.0f, 0.0f }, glm::vec3 velocity = { 5.0f, 5.0f, 5.0f });
    void SpawnBurst(entt::entity entity, const ParticleBurst& burst);
    void SpawnBurst(entt::entity entity, uint32_t count, float maxInterval, float startTime = 0.0f, bool loop = true, uint32_t cycles = 0);

private:
    std::shared_ptr<GraphicsContext> _context;
    ECSModule* _ecs = nullptr;
    PhysicsModule* _physics = nullptr;

    DataStore<EmitterPresetSettings> _emitterPresets;
    std::unordered_map<std::string, ResourceHandle<GPUImage>> _emitterImages;
    uint32_t emitterCount = 0;

    ResourceHandle<GPUImage>& GetEmitterImage(std::string fileName, bool& imageFound);
    bool SetEmitterPresetImage(EmitterPreset& preset);

    friend class ParticleEditor;
};