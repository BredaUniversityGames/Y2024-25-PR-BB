#pragma once

#include <cstddef>
#include <entt/entity/entity.hpp>
#include "imgui_entt_entity_editor.hpp"

struct RelationshipComponent
{
    size_t childrenCount = 0;

    entt::entity first = entt::null; // First child if any
    entt::entity prev = entt::null; // Previous sibling
    entt::entity next = entt::null; // Next sibling
    entt::entity parent = entt::null;
};

namespace MM
{
template <>
void ComponentEditorWidget<RelationshipComponent>(entt::registry& reg, entt::registry::entity_type e);
}