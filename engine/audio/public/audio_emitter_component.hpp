#pragma once

#include "audio_common.hpp"
#include "imgui_entt_entity_editor.hpp"

struct AudioEmitterComponent
{
    std::vector<SoundInstance> _soundIds {};
    std::vector<EventInstanceID> _eventIds {};
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AudioEmitterComponent>(entt::registry& reg, entt::registry::entity_type e);
}