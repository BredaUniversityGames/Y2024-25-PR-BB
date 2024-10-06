#pragma once

#include "entity/registry.hpp"

class System;

class ECS
{
public:
    ECS();
    ~ECS();

    ECS(ECS&&) = delete;
    ECS(const ECS&) = delete;

    ECS& operator=(ECS&&) = delete;
    ECS& operator=(const ECS&) = delete;

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
    T* system = new T(std::forward<Args>(args)...);

    _systems.emplace_back(std::unique_ptr<System>(system));

    spdlog::info("{}, created", typeid(*system).name());
}