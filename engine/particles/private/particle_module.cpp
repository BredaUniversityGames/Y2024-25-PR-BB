#include "particle_module.hpp"

#include "engine.hpp"
#include "old_engine.hpp"
#include "renderer.hpp"
#include <memory>

#include <renderer_module.hpp>

ModuleTickOrder ParticleModule::Init(Engine& engine)
{
    auto ecs = engine.GetModule<OldEngine>().GetECS();
    auto renderer = engine.GetModule<RendererModule>().GetRenderer();
    _particleInterface = std::make_unique<ParticleInterface>(renderer->GetContext(), ecs);

    return ModuleTickOrder::ePostTick;
}