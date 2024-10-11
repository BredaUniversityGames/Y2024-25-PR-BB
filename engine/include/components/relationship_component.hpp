#pragma once
#include <entity/entity.hpp>

struct RelationshipComponent
{
    size_t _children = 0;

    entt::entity _first = entt::null; // First child if any
    entt::entity _prev = entt::null; // Previous sibling
    entt::entity _next = entt::null; // Next sibling
    entt::entity _parent = entt::null;
};
