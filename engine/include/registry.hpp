#pragma once

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#include "entt.hpp"

#pragma clang diagnostic pop

#include "mesh.hpp"

struct SceneDescription;
class System;

class Registry
{
public:
    Registry();
    ~Registry();

    Registry(Registry&&) = delete;
    Registry(const Registry&) = delete;

    Registry& operator=(Registry&&) = delete;
    Registry& operator=(const Registry&) = delete;

    void AddSystem(std::unique_ptr<System> system);

    void UpdateSystems(SceneDescription& scene, float dt);

    void RenderSystems(const SceneDescription& scene);

    entt::entity Create();
    entt::entity Create(entt::entity hint);

    // TODO: add recursive child destruction
    void Destroy(entt::entity entity);

    bool IsValid(entt::entity entity);

    template <typename T>
    bool HasComponent(entt::entity entity);

    template <typename T, typename... Args>
    decltype(auto) AddComponent(entt::entity toEntity, Args&&... args);

    template <typename T>
    entt::registry::storage_for_type<T>& Storage() { return _registry.storage<T>(); }

    template <typename T>
    const entt::registry::storage_for_type<T>* Storage() const { return _registry.storage<T>(); }

    entt::registry _registry {};

    std::vector<std::unique_ptr<System>> _systems {};

    class IsDestroyedTag
    {
    };
};

template <typename T>
bool Registry::HasComponent(entt::entity entity)
{
    auto* storage = Storage<T>();
    return storage != nullptr && storage->contains(entity);
}
template <typename T, typename... Args>
decltype(auto) Registry::AddComponent(entt::entity toEntity, Args&&... args)
{
    return _registry.emplace<T>(toEntity, std::forward<Args>(args)...);
}