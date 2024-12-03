#include "particle_module.hpp"

#include "engine.hpp"
#include "old_engine.hpp"
#include "renderer.hpp"

#include <renderer_module.hpp>

ModuleTickOrder ParticleModule::Init(Engine& engine)
{
    auto ecs = engine.GetModule<OldEngine>().GetECS();
    auto renderer = engine.GetModule<RendererModule>().GetRenderer();
    _particleInterface = std::make_unique<ParticleInterface>(renderer->GetContext(), ecs);

    return ModuleTickOrder::ePreRender;
}

void ParticleModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _particleInterface.reset();
}

void ParticleModule::Tick(MAYBE_UNUSED Engine& engine)
{
}