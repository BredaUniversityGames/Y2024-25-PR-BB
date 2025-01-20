#pragma once
#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>

// Add more variables for cheats here
struct CheatsComponent
{
    bool noClip = false;
};
namespace EnttEditor
{
template <>
void ComponentEditorWidget<CheatsComponent>(entt::registry& reg, entt::registry::entity_type e);
}