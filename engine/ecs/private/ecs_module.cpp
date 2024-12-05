#include "ecs_module.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_helpers.hpp"
#include "time_module.hpp"

ModuleTickOrder ECSModule::Init(MAYBE_UNUSED Engine& engine)
{
    TransformHelpers::SubscribeToEvents(registry);
    RelationshipHelpers::SubscribeToEvents(registry);
    return ModuleTickOrder::eTick;
}

void ECSModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    TransformHelpers::UnsubscribeToEvents(registry);
    RelationshipHelpers::UnsubscribeToEvents(registry);
}

void ECSModule::Tick(Engine& engine)
{
    auto dt = engine.GetModule<TimeModule>().GetDeltatime().count();

    RemovedDestroyed();
    UpdateSystems(dt);
    RenderSystems();
}

void ECSModule::UpdateSystems(const float dt)
{
    for (auto& system : systems)
    {
        system->Update(*this, dt);
    }
}
void ECSModule::RenderSystems() const
{
    for (const auto& system : systems)
    {
        system->Render(*this);
    }
}
void ECSModule::RemovedDestroyed()
{
    const auto toDestroy = registry.view<DeleteTag>();
    for (const entt::entity entity : toDestroy)
    {
        registry.destroy(entity);
    }
}

void ECSModule::DestroyEntity(entt::entity entity)
{
    assert(registry.valid(entity));
    registry.emplace_or_replace<DeleteTag>(entity);
}