#include "components/relationship_helpers.hpp"

#include <entity/registry.hpp>
#include "components/relationship_component.hpp"

void RelationshipHelpers::SetParent(entt::registry& reg, entt::entity entity, entt::entity parent)
{
    assert(reg.valid(entity));
    assert(reg.valid(parent));
    // Remove from possible previous parent
    RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(entity);
    if (childRelationship._parent != entt::null)
    {
        DetachChild(reg, childRelationship._parent, entity);
    }

    AttachChild(reg, parent, entity);
}
void RelationshipHelpers::AttachChild(entt::registry& reg, entt::entity entity, entt::entity child)
{
    RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(entity);
    RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(child);

    if (childRelationship._parent != entt::null)
    {
        DetachChild(reg, childRelationship._parent, child);
    }

    if (parentRelationship._children == 0)
    {
        parentRelationship._first = child;
    }
    else
    {
        RelationshipComponent& firstChild = reg.get<RelationshipComponent>(parentRelationship._first);

        firstChild._prev = child;
        parentRelationship._first = child;
    }

    ++parentRelationship._children;
}
void RelationshipHelpers::DetachChild(entt::registry& reg, entt::entity entity, entt::entity child)
{
    RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(entity);
    RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(child);
    if (parentRelationship._first == child)
    {
        parentRelationship._first = childRelationship._next;
    }

    // Siblings
    if (childRelationship._prev != entt::null)
    {
        RelationshipComponent& prev = reg.get<RelationshipComponent>(childRelationship._prev);

        prev._next = childRelationship._next;
    }

    if (childRelationship._next != entt::null)
    {
        RelationshipComponent& next = reg.get<RelationshipComponent>(childRelationship._next);

        next._prev = childRelationship._prev;
    }

    childRelationship._parent = entt::null;

    --parentRelationship._children;
}
void RelationshipHelpers::OnDestroyRelationship(entt::registry& reg, entt::entity entity)
{
    RelationshipComponent& relationship = reg.get<RelationshipComponent>(entity);

    // Has a parent
    if (relationship._parent != entt::null)
    {
        // Check if head of children
        RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(relationship._parent);

        if (parentRelationship._first == entity)
        {
            // Set parent._first to this._next
            // If this is the only child _next will be entt::null
            parentRelationship._first = relationship._next;
        }
        // Decrement the parent's child counter
        --parentRelationship._children;
    }

    // Siblings
    if (relationship._prev != entt::null)
    {
        RelationshipComponent& prev = reg.get<RelationshipComponent>(relationship._prev);

        prev._next = relationship._next;
    }

    if (relationship._next != entt::null)
    {
        RelationshipComponent& next = reg.get<RelationshipComponent>(relationship._next);

        next._prev = relationship._prev;
    }

    if (relationship._children > 0)
    {
        entt::entity current = relationship._first;
        // Don't decrement this relationship components child counter or the loop would end early
        for (size_t i {}; i < relationship._children; ++i)
        {
            RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(current);

            current = childRelationship._next;

            // Parent has been removed so siblings should no longer be connected
            childRelationship._parent = entt::null;
            childRelationship._prev = entt::null;
            childRelationship._next = entt::null;
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