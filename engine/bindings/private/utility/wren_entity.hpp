#pragma once
#include <entt/entity/registry.hpp>
#include <optional>

template <typename T>
struct WrenComponent;

struct WrenEntity
{
    entt::entity entity;
    entt::registry* registry;

    template <typename T>
    std::optional<WrenComponent<T>> GetComponent();

    template <typename T>
    WrenComponent<T> AddComponent();

    entt::entity GetEntity() { return entity; }
};

template <typename T>
struct WrenComponent
{
    WrenEntity entity {};
    T* component {};
};

template <typename T>
WrenComponent<T> WrenEntity::AddComponent()
{
    registry->emplace_or_replace<T>(entity);
    return WrenComponent<T> { WrenEntity { entity, registry }, &registry->get<T>(entity) };
};

template <typename T>
std::optional<WrenComponent<T>> WrenEntity::GetComponent()
{
    if (auto* out = registry->try_get<T>(entity))
    {
        return WrenComponent<T> { WrenEntity { entity, registry }, out };
    }
    return std::nullopt;
};