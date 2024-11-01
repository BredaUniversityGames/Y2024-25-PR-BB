#pragma once

#include <cstdint>
#include <vector>

class ECS;
struct Emitter;

class ParticleInterface
{
public:
    ParticleInterface(ECS& ecs);

    enum class EmitterPreset
    {
        eTest = 0,
        eNone
    };

    void SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit = 1);

private:
    ECS& _ecs;
    std::vector<Emitter> _emitterPresets;
};