#pragma once

#include "resource_manager.hpp"

#include "particle_util.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class ECSModule;
class GraphicsContext;
struct GPUImage;

class ParticleInterface
{
public:
    ParticleInterface(const std::shared_ptr<GraphicsContext>& context, ECSModule& ecs);
    ~ParticleInterface() = default;

    enum class EmitterPreset
    {
        eTest = 0,
        eNone
    };

    void LoadEmitterPresets();
    void SpawnEmitter(EmitterPreset emitterPreset, uint32_t timesToEmit = 1);

private:
    std::shared_ptr<GraphicsContext> _context;
    ECSModule& _ecs;

    std::vector<Emitter> _emitterPresets;
    std::vector<ResourceHandle<GPUImage>> _emitterImages;

    // temporary solution
    uint32_t LoadEmitterImage(const char* imagePath);
};