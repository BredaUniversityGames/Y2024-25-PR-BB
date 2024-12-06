#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <string_view>

class NameComponent
{
public:
    std::string name;

    static std::string_view GetDisplayName(const entt::registry& registry, entt::entity entity);
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<NameComponent>(entt::registry& reg, entt::registry::entity_type e);
}