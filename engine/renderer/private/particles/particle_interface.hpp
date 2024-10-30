#pragma once

#include <cstdint>
#include <vector>
#include "particles/particle_util.hpp"

class ECS;

class ParticleInterface
{
public:
    ParticleInterface(const std::shared_ptr<ECS> ecs);
    ~ParticleInterface() = default;

    enum class EmitterPreset
    {
        eTest = 0,
        eNone
    };

    void SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit = 1);

private:
    const std::shared_ptr<ECS> _ecs;
    std::vector<Emitter> _emitterPresets;
};