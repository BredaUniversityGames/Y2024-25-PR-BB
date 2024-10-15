#include "particles/particle_util.hpp"
#include "particles/particle_interface.hpp"

#include "ECS.hpp"
#include "particles/emitter_component.hpp"

ParticleInterface::ParticleInterface(ECS& ecs)
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
        auto entity = _ecs._registry.create();
        EmitterComponent emitterComponent;
        _ecs._registry.emplace<EmitterComponent>(entity, emitterComponent);
    }
}

void ParticleInterface::SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit)
{
    auto view = _ecs._registry.view<EmitterComponent>();
    for (auto entity : view)
    {
        auto& emitterComponent = _ecs._registry.get<EmitterComponent>(entity);
        if (emitterComponent.timesToEmit == 0)
        {
            emitterComponent.timesToEmit = timesToEmit;
            emitterComponent.emitter = _emitterPresets[static_cast<int>(emitterPreset)];
            break;
        }
    }
}
