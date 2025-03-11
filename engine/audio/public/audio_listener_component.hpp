#pragma once

#include "imgui_entt_entity_editor.hpp"

struct AudioListenerComponent
{
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AudioListenerComponent>(entt::registry& reg, entt::registry::entity_type e);
}