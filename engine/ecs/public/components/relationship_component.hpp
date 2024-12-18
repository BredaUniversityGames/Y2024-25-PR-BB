#pragma once

#include <entt/entity/registry.hpp>
#include <imgui_entt_entity_editor.hpp>

// #define RELATIONSHIP_USE_VECTOR

#if defined(RELATIONSHIP_USE_VECTOR)
struct RelationshipComponent
{
    size_t layer = 0;
    std::vector<entt::entity> entities;
};
#else
struct RelationshipComponent
{
    size_t layer = 0;
    size_t childrenCount = 0;

    entt::entity first = entt::null; // First child if any
    entt::entity prev = entt::null; // Previous sibling
    entt::entity next = entt::null; // Next sibling
    entt::entity parent = entt::null;
};
#endif

namespace RelationshipHelpers
{
void SetParent(entt::registry& reg, entt::entity entity, entt::entity parent);

void AttachChild(entt::registry& reg, entt::entity entity, entt::entity child);
void DetachChild(entt::registry& reg, entt::entity entity, entt::entity child);

void OnDestroyRelationship(entt::registry& reg, entt::entity entity);

void SubscribeToEvents(entt::registry& reg);
void UnsubscribeToEvents(entt::registry& reg);
}

namespace EnttEditor
{
template <>
void ComponentEditorWidget<RelationshipComponent>(entt::registry& reg, entt::registry::entity_type e);
}