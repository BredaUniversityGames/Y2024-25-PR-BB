#pragma once

#include <entt/entity/registry.hpp>
#include <imgui_entt_entity_editor.hpp>

struct RelationshipComponent
{
    size_t layer = 0;

    entt::entity firstChild = entt::null; // Last child
    entt::entity prevSibling = entt::null; // Previous sibling
    entt::entity nextSibling = entt::null; // Next sibling
    entt::entity parent = entt::null;

    struct RelationshipIterator
    {
        RelationshipIterator(entt::registry& reg, entt::entity current)
            : reg(reg)
            , current(current)
        {
        }

        bool operator!=(const RelationshipIterator& other) const
        {
            return current != other.current;
        }

        RelationshipIterator& operator++()
        {
            current = reg.get<RelationshipComponent>(current).nextSibling;
            return *this;
        }

        entt::entity operator*() const
        {
            return current;
        }

    private:
        entt::registry& reg;
        entt::entity current;
    };

    struct RelationShipIterFacade
    {
        RelationShipIterFacade(const RelationshipComponent& comp, entt::registry& reg)
            : comp(comp)
            , reg(reg)
        {
        }

        RelationshipIterator begin() const { return { reg, comp.firstChild }; }
        RelationshipIterator end() const { return { reg, entt::null }; }

    private:
        const RelationshipComponent& comp;
        entt::registry& reg;
    };

    RelationShipIterFacade IterateChildren(entt::registry& registry) const
    {
        return { *this, registry };
    }
};

namespace RelationshipHelpers
{
void SetParent(entt::registry& reg, entt::entity entity, entt::entity parent);

// void AttachChild(entt::registry& reg, entt::entity entity, entt::entity child);
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