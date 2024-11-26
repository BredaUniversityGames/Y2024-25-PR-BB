#pragma once

#include <string>
#include <string_view>
#include <entt/entity/registry.hpp>

class NameComponent
{
public:
    std::string _name {};

    static std::string_view GetDisplayName(const entt::registry& registry, entt::entity entity);
};