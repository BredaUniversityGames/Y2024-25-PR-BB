#pragma once

#include "resource_manager.hpp"

#include "particle_util.hpp"
#include <cstdint>
#include <memory>
#include <vector>

class ECS;
class GraphicsContext;
struct GPUImage;

class ParticleInterface
{
public:
    ParticleInterface(const std::shared_ptr<GraphicsContext>& context, const std::shared_ptr<ECS>& ecs);
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
    std::shared_ptr<ECS> _ecs;

    std::vector<Emitter> _emitterPresets;
    std::vector<ResourceHandle<GPUImage>> _emitterImages;


    // temporary solution
    uint32_t LoadEmitterImage(const char* imagePath);
};