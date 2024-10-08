#pragma once

#include "entity/registry.hpp"
#include "systems/system.hpp"
#include <spdlog/spdlog.h>

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

    void UpdateSystems(float dt);

    void RenderSystems() const;

    void RemovedDestroyed();

    void DestroyEntity(entt::entity entity);

    entt::registry _registry {};

    std::vector<std::unique_ptr<System>> _systems {};

    class ToDestroy
    {
    };
};

template <typename T, typename... Args>
void ECS::AddSystem(Args&&... args)
{
    static_assert(std::is_base_of<System, T>::value, "Tried to add incorrect class as system");
    T* system = new T(std::forward<Args>(args)...);

    _systems.emplace_back(std::unique_ptr<System>(system));

    spdlog::info("{}, created", typeid(*system).name());
}