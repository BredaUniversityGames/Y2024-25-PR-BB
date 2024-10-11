#pragma once

#include <entity/entity.hpp>

class RelationshipHelpers
{
public:
    static void SetParent(entt::registry& reg, entt::entity entity, entt::entity parent);

    static void AttachChild(entt::registry& reg, entt::entity entity, entt::entity child);
    static void DetachChild(entt::registry& reg, entt::entity entity, entt::entity child);

    static void OnDestroyRelationship(entt::registry& reg, entt::entity entity);

    static void SubscribeToEvents(entt::registry& reg);
    static void UnsubscribeToEvents(entt::registry& reg);
};