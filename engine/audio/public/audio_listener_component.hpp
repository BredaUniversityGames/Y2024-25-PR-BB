#pragma once

#include "imgui_entt_entity_editor.hpp"

#include <entt/entt.hpp>

struct AudioListenerComponent
{
    // Set page size because component is empty, must be power of 2,
    // We won't have multiple listeners, but the option is there, so we keep it low
    static constexpr auto page_size = 64;
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AudioListenerComponent>(entt::registry& reg, entt::registry::entity_type e);
}