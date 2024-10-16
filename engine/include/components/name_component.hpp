#pragma once
#include <entt/entity/entity.hpp>
#include "imgui_entt_entity_editor.hpp"

class NameComponent
{
public:
    std::string _name {};

    static std::string_view GetDisplayName(const entt::registry& registry, entt::entity entity);
};

namespace MM
{
template <>
void ComponentEditorWidget<NameComponent>(entt::registry& reg, entt::registry::entity_type e);
}