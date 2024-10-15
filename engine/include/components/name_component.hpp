#pragma once
#include <entt/entity/entity.hpp>

class NameComponent
{
public:
    std::string _name {};

    static std::string_view GetDisplayName(const entt::registry& registry, entt::entity entity);
};