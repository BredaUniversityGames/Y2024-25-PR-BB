#pragma once

#include "vulkan_brain.hpp"

class ECS;
struct Emitter;

class ParticleInterface
{
public:
    ParticleInterface(const VulkanBrain& brain, ECS& ecs);
    ~ParticleInterface();

    enum class EmitterPreset
    {
        eTest = 0,
        eNone
    };

    void SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit = 1);

private:
    ECS& _ecs;
    const VulkanBrain& _brain;

    std::vector<Emitter> _emitterPresets;
    std::vector<ResourceHandle<Image>> _emitterImages;

    // temporary solution
    uint32_t LoadEmitterImage(const char* imagePath);
};