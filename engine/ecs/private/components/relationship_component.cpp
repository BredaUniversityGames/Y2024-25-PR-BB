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

    // TODO: maybe add some debug log or assert for these conditions
    if (childRelationship == nullptr)
        return;
    if (parentRelationship == childRelationship)
        return;

    Unparent(reg, entity);

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
        childRelationship->layer = parentRelationship->layer + 1;
    }
}

void RelationshipHelpers::Unparent(entt::registry& reg, entt::entity entity)
{
    RelationshipComponent* childRelationship = reg.try_get<RelationshipComponent>(entity);

    if (childRelationship == nullptr)
        return;

    RelationshipComponent* parentRelationship = reg.try_get<RelationshipComponent>(childRelationship->parent);

    if (parentRelationship == nullptr)
        return;

    auto* prevSibling = reg.try_get<RelationshipComponent>(childRelationship->prevSibling);
    auto* nextSibling = reg.try_get<RelationshipComponent>(childRelationship->nextSibling);

    if (parentRelationship->firstChild == entity)
    {
        parentRelationship->firstChild = childRelationship->nextSibling;
    }

    if (prevSibling)
    {
        prevSibling->nextSibling = childRelationship->nextSibling;
    }

    if (nextSibling)
    {
        nextSibling->prevSibling = childRelationship->prevSibling;
    }

    childRelationship->prevSibling = entt::null;
    childRelationship->nextSibling = entt::null;
    childRelationship->layer = 0;
    childRelationship->parent = entt::null;
}

void OnDestroyRelationship(entt::registry& reg, entt::entity entity)
{
    RelationshipComponent& relationship = reg.get<RelationshipComponent>(entity);
    RelationshipHelpers::Unparent(reg, entity);

    auto f = relationship.IterateChildren(reg);
    for (auto it = f.begin(); it != f.end();)
    {
        entt::entity child = *it;
        ++it;

        auto& name = reg.get<NameComponent>(child);
        bblog::info("{}", name.name);

        reg.destroy(child);
    }
}

void RelationshipHelpers::SortRegistryHierarchy(entt::registry& reg)
{
    auto sort = [](const RelationshipComponent& lhs, const RelationshipComponent& rhs)
    {
        return lhs.layer < rhs.layer;
    };

    reg.sort<RelationshipComponent>(sort);
}

void RelationshipHelpers::SubscribeToEvents(entt::registry& reg)
{
    reg.on_destroy<RelationshipComponent>().connect<&OnDestroyRelationship>();
}
void RelationshipHelpers::UnsubscribeToEvents(entt::registry& reg)
{
    reg.on_destroy<RelationshipComponent>().disconnect<&OnDestroyRelationship>();
}
