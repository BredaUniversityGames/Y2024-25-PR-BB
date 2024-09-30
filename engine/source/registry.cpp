#include "registry.hpp"

#include "systems/system.hpp"

Registry::Registry() = default;
Registry::~Registry() = default;

void Registry::AddSystem(std::unique_ptr<System> system)
{
    // Check for duplicate systems?
    // Do update interval reordering?
    _systems.emplace_back(std::move(system));
}
void Registry::UpdateSystems(SceneDescription& scene, const float dt)
{
    for (auto& system : _systems)
    {
        system->Update(scene, dt);
    }
}
void Registry::RenderSystems(const SceneDescription& scene)
{
    for (auto& system : _systems)
    {
        system->Render(scene);
    }
}
entt::entity Registry::Create()
{
    return _registry.create();
}
entt::entity Registry::Create(entt::entity hint)
{
    return _registry.create(hint);
}
void Registry::Destroy(entt::entity entity)
{
    if (!IsValid(entity) || HasComponent<IsDestroyedTag>(entity))
    {
        return;
    }

    AddComponent<IsDestroyedTag>(entity);

    // if destroy children
    // Traverse hierarchy and recurse this function
}
bool Registry::IsValid(entt::entity entity)
{
    return _registry.valid(entity);
}