#include "components/relationship_helpers.hpp"

#include "components/relationship_component.hpp"
#include <entt/entity/registry.hpp>

void RelationshipHelpers::SetParent(entt::registry& reg, entt::entity entity, entt::entity parent)
{
    assert(reg.valid(entity));
    assert(reg.valid(parent));

    AttachChild(reg, parent, entity);
}
void RelationshipHelpers::AttachChild(entt::registry& reg, entt::entity entity, entt::entity child)
{
    RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(entity);
    RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(child);

    if (childRelationship.parent != entt::null)
    {
        DetachChild(reg, childRelationship.parent, child);
    }

    if (parentRelationship.childrenCount == 0)
    {
        parentRelationship.first = child;
    }
    else
    {
        RelationshipComponent& firstChild = reg.get<RelationshipComponent>(parentRelationship.first);

        firstChild.prev = child;
        childRelationship.next = parentRelationship.first;
        parentRelationship.first = child;
    }

    childRelationship.parent = entity;
    ++parentRelationship.childrenCount;
}
void RelationshipHelpers::DetachChild(entt::registry& reg, entt::entity entity, entt::entity child)
{
    RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(entity);
    RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(child);
    if (parentRelationship.first == child)
    {
        parentRelationship.first = childRelationship.next;
    }

    // Siblings
    if (childRelationship.prev != entt::null)
    {
        RelationshipComponent& prev = reg.get<RelationshipComponent>(childRelationship.prev);

        prev.next = childRelationship.next;
    }

    if (childRelationship.next != entt::null)
    {
        RelationshipComponent& next = reg.get<RelationshipComponent>(childRelationship.next);

        next.prev = childRelationship.prev;
    }

    childRelationship.parent = entt::null;

    --parentRelationship.childrenCount;
}
void RelationshipHelpers::OnDestroyRelationship(entt::registry& reg, entt::entity entity)
{
    RelationshipComponent& relationship = reg.get<RelationshipComponent>(entity);

    // Has a parent
    if (relationship.parent != entt::null)
    {
        // Check if head of children
        RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(relationship.parent);

        if (parentRelationship.first == entity)
        {
            // Set parent._first to this._next
            // If this is the only child _next will be entt::null
            parentRelationship.first = relationship.next;
        }
        // Decrement the parent's child counter
        --parentRelationship.childrenCount;
    }

    // Siblings
    if (relationship.prev != entt::null)
    {
        RelationshipComponent& prev = reg.get<RelationshipComponent>(relationship.prev);

        prev.next = relationship.next;
    }

    if (relationship.next != entt::null)
    {
        RelationshipComponent& next = reg.get<RelationshipComponent>(relationship.next);

        next.prev = relationship.prev;
    }

    if (relationship.childrenCount > 0)
    {
        entt::entity current = relationship.first;
        // Don't decrement this relationship components child counter or the loop would end early
        for (size_t i {}; i < relationship.childrenCount; ++i)
        {
            RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(current);

            current = childRelationship.next;

            // Parent has been removed so siblings should no longer be connected
            childRelationship.parent = entt::null;
            childRelationship.prev = entt::null;
            childRelationship.next = entt::null;
        }
    }
}
void RelationshipHelpers::SubscribeToEvents(entt::registry& reg)
{
    reg.on_destroy<RelationshipComponent>().connect<&OnDestroyRelationship>();
}
void RelationshipHelpers::UnsubscribeToEvents(entt::registry& reg)
{
    reg.on_destroy<RelationshipComponent>().disconnect<&OnDestroyRelationship>();
}