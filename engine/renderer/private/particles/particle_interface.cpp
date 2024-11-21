#include "particles/particle_interface.hpp"
#include "particles/particle_util.hpp"

#include "components/name_component.hpp"
#include "ecs.hpp"
#include "particles/emitter_component.hpp"

ParticleInterface::ParticleInterface(const std::shared_ptr<ECS>& ecs)
    : _ecs(ecs)
{
    // TODO: later, serialize emitter presets and load from file here
    // hardcoded test emitter preset for now
    Emitter emitter;
    emitter.position = glm::vec3(1.0f, 2.0f, 3.0f);
    emitter.count = 3;
    emitter.velocity = glm::vec3(1.0f);
    emitter.mass = 2.0f;
    emitter.rotationVelocity = glm::vec3(1.0f);
    emitter.maxLife = 5.0f;
    _emitterPresets.emplace_back(emitter);

    // fill ECS with emitters
    for (size_t i = 0; i < MAX_EMITTERS; i++)
    {
        auto entity = _ecs->registry.create();
        EmitterComponent emitterComponent;
        _ecs->registry.emplace<EmitterComponent>(entity, emitterComponent);
        auto& name = _ecs->registry.emplace<NameComponent>(entity);
        name.name = "Particle Emitter " + std::to_string(i);
    }
}

void ParticleInterface::SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit)
{
    auto view = _ecs->registry.view<EmitterComponent>();
    for (auto entity : view)
    {
        auto& emitterComponent = _ecs->registry.get<EmitterComponent>(entity);
        if (emitterComponent.timesToEmit == 0)
        {
            emitterComponent.timesToEmit = timesToEmit;
            emitterComponent.emitter = _emitterPresets[static_cast<int>(emitterPreset)];
            break;
        }
    }
}