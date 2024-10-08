#pragma once

#include "model_loader.hpp"
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

    void LoadGLTFIntoScene(std::string_view path, ModelLoader& loader, BatchBuffer& batchBuffer /* temp, should not have to be included when mesh creation goes through resource manager*/);
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