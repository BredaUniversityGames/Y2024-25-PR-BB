#pragma once

#include "particle_util.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class ECS;

class ParticleInterface
{
public:
    ParticleInterface(const std::shared_ptr<ECS>& ecs);

    enum class EmitterPreset
    {
        eTest = 0,
        eNone
    };

    void SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit = 1);

private:
    std::shared_ptr<ECS> _ecs;
    std::vector<Emitter> _emitterPresets;
};