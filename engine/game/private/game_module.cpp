#include "game_module.hpp"

#include <ecs_module.hpp>
#include <lifetime_system.hpp>

ModuleTickOrder GameModule::Init(Engine& engine)
{
    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    return ModuleTickOrder::eTick;
}
void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}
void GameModule::Tick(MAYBE_UNUSED Engine& engine)
{
}