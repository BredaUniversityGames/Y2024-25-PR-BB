#include "inspector_module.hpp"

ModuleTickOrder InspectorModule::Init(Engine& engine)
{
    return ModuleTickOrder::ePostTick;
}

void InspectorModule::Shutdown(Engine& engine)
{
}

void InspectorModule::Tick(Engine& engine)
{
}