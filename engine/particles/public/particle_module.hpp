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

class ParticleModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
    void Tick(MAYBE_UNUSED Engine& engine) override;

public:
    ParticleModule() = default;
    ~ParticleModule() override = default;

    enum class EmitterPresetID
    {
        eTest = 0,
        eNone
    };

    void LoadEmitterPresets();
    void SpawnEmitter(entt::entity entity, EmitterPresetID emitterPreset, bool emitOnce, bool isActive = true);

private:
    std::shared_ptr<GraphicsContext> _context;
    ECSModule* _ecs;
    PhysicsModule* _physics;

    struct EmitterPreset
    {
        glm::vec3 size = { 1.0f, 1.0f, 0.0f }; // 2d size + velocity
        float mass = 1.0f;
        glm::vec2 rotationVelocity = { 0.0f, 0.0f }; // angle + velocity
        float maxLife = 5.0f;
        float emitDelay = 1.0f;
        uint32_t count = 0;
        uint32_t materialIndex = 0;
        ParticleType type = ParticleType::eBillboard;
    };

    std::vector<EmitterPreset> _emitterPresets;
    std::vector<ResourceHandle<GPUImage>> _emitterImages;

    // temporary solution
    uint32_t LoadEmitterImage(const char* imagePath);
};