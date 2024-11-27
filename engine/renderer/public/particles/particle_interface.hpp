#pragma once

#include "vulkan_brain.hpp"
#include "particle_util.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class ECS;

class ParticleInterface
{
public:
    ParticleInterface(const VulkanBrain& brain, const std::shared_ptr<ECS>& ecs);
    ~ParticleInterface();

    enum class EmitterPreset
    {
        eTest = 0,
        eNone
    };

    void SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit = 1);

private:
    std::shared_ptr<ECS> _ecs;
    const VulkanBrain& _brain;

    std::vector<Emitter> _emitterPresets;
    std::vector<ResourceHandle<Image>> _emitterImages;

    // temporary solution
    uint32_t LoadEmitterImage(const char* imagePath);
};