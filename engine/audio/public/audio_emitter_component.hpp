#pragma once

#include "audio_common.hpp"
#include "imgui_entt_entity_editor.hpp"

// Add audio to this component by emplace_back(PlaySound/PlayEvent)
// Do not add the same sound to multiple emitters, it will sound weird
// SoundInstances and EvenInstances are automatically removed if they aren't looping
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