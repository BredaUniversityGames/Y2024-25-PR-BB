#pragma once
#include <entity/entity.hpp>

class NameComponent
{
public:
    std::string _name {};

    static std::string GetDisplayName(const entt::registry& registry, entt::entity entity);
};