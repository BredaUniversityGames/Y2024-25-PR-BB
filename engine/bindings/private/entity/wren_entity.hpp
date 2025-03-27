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
    WrenComponent<T> AddDefaultComponent();

    template <typename T>
    WrenComponent<T> AddComponent(const T& component);

    template <typename T>
    void AddTag();

    template <typename T>
    bool HasComponent();

    bool IsValid();
};

template <typename T>
struct WrenComponent
{
    WrenEntity entity {};
    T* component {};
};

template <typename T>
WrenComponent<T> WrenEntity::AddDefaultComponent()
{
    registry->emplace_or_replace<T>(entity);
    return WrenComponent<T> { WrenEntity { entity, registry }, &registry->get<T>(entity) };
}

template <typename T>
WrenComponent<T> WrenEntity::AddComponent(const T& component)
{
    registry->emplace_or_replace<T>(entity, component);
    return WrenComponent<T> { WrenEntity { entity, registry }, &registry->get<T>(entity) };
};

template <typename T>
void WrenEntity::AddTag()
{
    registry->emplace_or_replace<T>(entity);
}

template <typename T>
bool WrenEntity::HasComponent()
{
    return registry->all_of<T>(entity);
}

template <typename T>
std::optional<WrenComponent<T>> WrenEntity::GetComponent()
{
    if (auto* out = registry->try_get<T>(entity))
    {
        return WrenComponent<T> { WrenEntity { entity, registry }, out };
    }
    return std::nullopt;
};

inline bool WrenEntity::IsValid()
{
    return registry->valid(entity);
}