#pragma once

#include <entt/entity/registry.hpp>

namespace RelationshipHelpers
{
void SetParent(entt::registry& reg, entt::entity entity, entt::entity parent);

void AttachChild(entt::registry& reg, entt::entity entity, entt::entity child);
void DetachChild(entt::registry& reg, entt::entity entity, entt::entity child);

void OnDestroyRelationship(entt::registry& reg, entt::entity entity);

void SubscribeToEvents(entt::registry& reg);
void UnsubscribeToEvents(entt::registry& reg);
}