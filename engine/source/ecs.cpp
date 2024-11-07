#include "ecs.hpp"

ECS::ECS() = default;
ECS::~ECS() = default;

void ECS::UpdateSystems(const float dt)
{
    for (auto& system : systems)
    {
        system->Update(*this, dt);
    }
}
void ECS::RenderSystems() const
{
    for (const auto& system : systems)
    {
        system->Render(*this);
    }
}
void ECS::RemovedDestroyed()
{
    const auto toDestroy = registry.view<ToDestroy>();
    for (const entt::entity entity : toDestroy)
    {
        registry.destroy(entity);
    }
}

void ECS::DestroyEntity(entt::entity entity)
{
    assert(registry.valid(entity));
    registry.emplace_or_replace<ToDestroy>(entity);
}
