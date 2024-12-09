#pragma once
#include <entt/entity/registry.hpp>
#include <optional>

struct WrenEntity
{
    entt::entity entity;
    entt::registry* registry;

    template <typename T>
    std::optional<T*> GetComponent()
    {
        if (auto* out = registry->try_get<T>(entity))
        {
            return out;
        }
        return std::nullopt;
    };

    template <typename T>
    T* AddComponent()
    {
        registry->emplace_or_replace<T>(entity);
        return &registry->get<T>(entity);
    };
};

template <typename T>
struct WrenComponent
{
    T* component {};
    WrenEntity entity {};
};