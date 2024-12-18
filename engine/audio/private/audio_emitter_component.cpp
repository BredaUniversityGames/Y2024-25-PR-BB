#include "audio_emitter_component.hpp"
template <>
void EnttEditor::ComponentEditorWidget<AudioEmitterComponent, entt::entity>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<AudioEmitterComponent>(e);

    ImGui::Text("Active sound count: %zu", comp._soundIds.size());
    ImGui::Text("Active event count: %zu", comp._eventIds.size());
}