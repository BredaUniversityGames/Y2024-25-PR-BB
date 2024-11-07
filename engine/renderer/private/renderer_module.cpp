#include "renderer_module.hpp"

RendererModule::RendererModule()
{
}

ModuleTickOrder RendererModule::Init(Engine& engine)
{
    return ModuleTickOrder::eRender;
}

void RendererModule::Shutdown(Engine& engine)
{
}

void RendererModule::Tick(Engine& engine)
{
}
