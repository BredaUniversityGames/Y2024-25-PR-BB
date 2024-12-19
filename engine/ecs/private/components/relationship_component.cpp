#include "components/relationship_component.hpp"

#include <imgui.h>

namespace EnttEditor
{
template <>
void ComponentEditorWidget<RelationshipComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<RelationshipComponent>(e);

    ImGui::Text("Children Count: %d", static_cast<int>(e));
    ImGui::Text("Parent: %d", static_cast<int>(comp.parent));
    ImGui::Text("First: %d", static_cast<int>(comp.firstChild));
    ImGui::Text("Prev: %d", static_cast<int>(comp.prevSibling));
    ImGui::Text("Next: %d", static_cast<int>(comp.nextSibling));
}
}

void RelationshipHelpers::SetParent(entt::registry& reg, entt::entity entity, entt::entity parent)
{
    RelationshipComponent* parentRelationship = reg.try_get<RelationshipComponent>(parent);
    RelationshipComponent* childRelationship = reg.try_get<RelationshipComponent>(entity);

    // TODO: maybe add some debug or assert for these conditions
    if (childRelationship == nullptr)
        return;
    if (parentRelationship == childRelationship)
        return;

    DetachChild(reg, childRelationship->parent, entity);

    if (parentRelationship)
    {
        if (auto* firstChild = reg.try_get<RelationshipComponent>(parentRelationship->firstChild))
        {
            firstChild->prevSibling = entity;
            childRelationship->nextSibling = parentRelationship->firstChild;
            parentRelationship->firstChild = entity;
        }
        childRelationship->parent = parent;
        parentRelationship->firstChild = entity;
    }
}
// void RelationshipHelpers::AttachChild(entt::registry& reg, entt::entity entity, entt::entity child)
// {
//     RelationshipComponent& parentRelationship = reg.get<RelationshipComponent>(entity);
//     RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(child);
//
//     if (childRelationship.parent != entt::null)
//     {
//         DetachChild(reg, childRelationship.parent, child);
//     }
//
//     if (parentRelationship.childrenCount == 0)
//     {
//         parentRelationship.first = child;
//     }
//     else
//     {
//         RelationshipComponent& firstChild = reg.get<RelationshipComponent>(parentRelationship.first);
//
//         firstChild.prev = child;
//         childRelationship.next = parentRelationship.first;
//         parentRelationship.first = child;
//     }
//
//     childRelationship.parent = entity;
//     ++parentRelationship.childrenCount;
// }
void RelationshipHelpers::DetachChild(entt::registry& reg, entt::entity parent, entt::entity child)
{
    RelationshipComponent* parentRelationship = reg.try_get<RelationshipComponent>(parent);
    RelationshipComponent* childRelationship = reg.try_get<RelationshipComponent>(child);

    if (parentRelationship == nullptr)
        return;
    if (childRelationship == nullptr)
        return;

    auto* prev_sibling = reg.try_get<RelationshipComponent>(childRelationship->prevSibling);
    auto* next_sibling = reg.try_get<RelationshipComponent>(childRelationship->nextSibling);

    if (parentRelationship->firstChild == child)
    {
        parentRelationship->firstChild = childRelationship->nextSibling;
    }

    if (prev_sibling)
    {
        prev_sibling->prevSibling = childRelationship->prevSibling;
    }

    if (next_sibling)
    {
        next_sibling->nextSibling = childRelationship->nextSibling;
    }

    childRelationship->parent = entt::null;
}

void RelationshipHelpers::OnDestroyRelationship(entt::registry& reg, entt::entity entity)
{
    RelationshipComponent& relationship = reg.get<RelationshipComponent>(entity);
    DetachChild(reg, entity, relationship.parent);

    for (auto e : relationship.IterateChildren(reg))
    {
        DetachChild(reg, e, entity);
    }

    // // Siblings
    // if (relationship.prev != entt::null)
    // {
    //     RelationshipComponent& prev = reg.get<RelationshipComponent>(relationship.prev);
    //
    //     prev.next = relationship.next;
    // }
    //
    // if (relationship.next != entt::null)
    // {
    //     RelationshipComponent& next = reg.get<RelationshipComponent>(relationship.next);
    //
    //     next.prev = relationship.prev;
    // }
    //
    // if (relationship.childrenCount > 0)
    // {
    //     entt::entity current = relationship.first;
    //     // Don't decrement this relationship components child counter or the loop would end early
    //     for (size_t i {}; i < relationship.childrenCount; ++i)
    //     {
    //         RelationshipComponent& childRelationship = reg.get<RelationshipComponent>(current);
    //
    //         current = childRelationship.next;
    //
    //         // Parent has been removed so siblings should no longer be connected
    //         childRelationship.parent = entt::null;
    //         childRelationship.prev = entt::null;
    //         childRelationship.next = entt::null;
    //     }
    // }
}
void RelationshipHelpers::SubscribeToEvents(entt::registry& reg)
{
    reg.on_destroy<RelationshipComponent>().connect<&OnDestroyRelationship>();
}
void RelationshipHelpers::UnsubscribeToEvents(entt::registry& reg)
{
    reg.on_destroy<RelationshipComponent>().disconnect<&OnDestroyRelationship>();
}
