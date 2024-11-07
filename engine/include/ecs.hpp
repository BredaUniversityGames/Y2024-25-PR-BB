#pragma once

#include "common.hpp"
#include "entity_serializer.hpp"
#include "entt/entity/registry.hpp"
#include "systems/system.hpp"
#include "log.hpp"
#include <assert.h>

class System;

class ECS
{
public:
    ECS();
    ~ECS();

    NON_COPYABLE(ECS);
    NON_MOVABLE(ECS);

    template <typename T, typename... Args>
    void AddSystem(Args&&... args);

    template <typename T>
    T& GetSystem();

    void UpdateSystems(float dt);
    void RenderSystems() const;
    void RemovedDestroyed();
    void DestroyEntity(entt::entity entity);

    entt::registry registry {};

    std::vector<std::unique_ptr<System>> systems {};

    class ToDestroy
    {
    };
};

template <typename T, typename... Args>
void ECS::AddSystem(Args&&... args)
{
    static_assert(std::is_base_of<System, T>::value, "Tried to add incorrect class as system");
    T* system = new T(std::forward<Args>(args)...);

    systems.emplace_back(std::unique_ptr<System>(system));

    spdlog::info("{}, created", typeid(*system).name());
}

CEREAL_CLASS_VERSION(ECS, 0);
template <class Archive>
void save(Archive& archive, ECS const& ecs, MAYBE_UNUSED uint32_t version)
{
    auto entityView = ecs.registry.view<entt::entity>();
    for (auto entity : entityView)
    {
        archive(EntitySerializer(ecs.registry, entity));
    }
}
template <typename T>
T& ECS::GetSystem()
{
    for (auto& s : systems)
    {
        T* found = dynamic_cast<T*>(s.get());
        if (found)
        {
            return *found;
        }
    }
    assert(false && "Could not find system");
    return *static_cast<T*>(nullptr); // This line will always fail
}
