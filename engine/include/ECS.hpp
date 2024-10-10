#pragma once

#include "entity/registry.hpp"
#include "systems/system.hpp"
#include <spdlog/spdlog.h>
#include "entity/snapshot.hpp"
#include <cereal/cereal.hpp>
#include <cereal/details/static_object.hpp>

namespace std::filesystem
{
class path;
}
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

    void WriteToFile(const std::filesystem::path& filePath);

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

class EntitySerializor
{
public:
    EntitySerializor(entt::registry& registry, entt::entity entity = entt::null)
        : _registry(registry)
        , _entity(entity)
    {
    }

    template <class Archive>
    void save(Archive& archive, uint32_t const version) const;

private:
    entt::registry& _registry;
    entt::entity _entity;
};

CEREAL_CLASS_VERSION(EntitySerializor, 0);
template <class Archive>
void EntitySerializor::save(Archive& archive, uint32_t const version) const
{
    x static auto trySaveComponent = [&]<typename T>()
    {
        if (auto component = _registry.try_get<T>(_entity); component != nullptr)
            archive(cereal::make_nvp(typeid(T).name(), *component));
    };

    if (version == 1)
    {
        trySaveComponent.template operator()<int>();
    }
}
