#include "particle_module.hpp"
#include "ecs_module.hpp"
#include "engine.hpp"
#include "particle_interface.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"

#include <memory>

ModuleTickOrder ParticleModule::Init(Engine& engine)
{
    auto& ecs = engine.GetModule<ECSModule>();
    auto renderer = engine.GetModule<RendererModule>().GetRenderer();
    _particleInterface = std::make_unique<ParticleInterface>(renderer->GetContext(), ecs);

    return ModuleTickOrder::ePostTick;
}