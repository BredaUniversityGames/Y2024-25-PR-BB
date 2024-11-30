#pragma once

#include "common.hpp"
#include "entity_serializer.hpp"
#include "entt/entity/registry.hpp"
#include "log.hpp"
#include "systems/system.hpp"
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
    T* GetSystem();

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
    systems.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    spdlog::info("{}, created", typeid(T).name());
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
T* ECS::GetSystem()
{
    for (auto& s : systems)
    {
        T* found = dynamic_cast<T*>(s.get());
        return found;
    }
    assert(false && "Could not find system");
    return nullptr;
}
