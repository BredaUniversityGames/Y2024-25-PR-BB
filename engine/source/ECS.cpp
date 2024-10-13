#include "ECS.hpp"

#include "entity_serializer.hpp"
#include "systems/system.hpp"

#include <filesystem>
#include <fstream>
#include <cereal/archives/json.hpp>

ECS::ECS() = default;
ECS::~ECS() = default;

void ECS::UpdateSystems(const float dt)
{
    for (auto& system : _systems)
    {
        system->Update(*this, dt);
    }
}
void ECS::RenderSystems() const
{
    for (const auto& system : _systems)
    {
        system->Render(*this);
    }
}
void ECS::RemovedDestroyed()
{
    const auto toDestroy = _registry.view<ToDestroy>();
    for (const entt::entity entity : toDestroy)
    {
        _registry.destroy(entity);
    }
}

void ECS::DestroyEntity(entt::entity entity)
{
    assert(_registry.valid(entity));
    _registry.emplace_or_replace<ToDestroy>(entity);
}
