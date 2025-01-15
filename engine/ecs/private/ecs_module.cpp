#include "ecs_module.hpp"
#include "components/name_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_helpers.hpp"
#include "scripting_module.hpp"
#include "systems/physics_system.hpp"
#include "time_module.hpp"

#include <tracy/Tracy.hpp>

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
    ZoneScoped;
    for (auto& system : systems)
    {
        ZoneScoped;
        std::string name = std::string(system->GetName()) + " Update";
        ZoneName(name.c_str(), 32);

        system->Update(*this, dt);
    }
}
void ECSModule::RenderSystems() const
{
    ZoneScoped;
    for (const auto& system : systems)
    {
        ZoneScoped;
        std::string name = std::string(system->GetName()) + " Render";
        ZoneName(name.c_str(), 32);

        system->Render(*this);
    }
}
void ECSModule::RemovedDestroyed()
{
    // TODO: should be somewhere else
    if (auto* physics = GetSystem<PhysicsSystem>())
    {
        physics->CleanUp();
    }

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