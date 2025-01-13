#pragma once

#include "imgui_entt_entity_editor.hpp"
#include "particle_util.hpp"
#include <entt/entity/registry.hpp>

struct ActiveEmitterTag
{
};

struct EmitterComponent
{
    bool emitOnce = true;
    float maxEmitDelay = 1.0f;
    float currentEmitDelay = 1.0f;
    ParticleType type = ParticleType::eBillboard;
    Emitter emitter;
    void Inspect(entt::registry& reg, entt::entity entity);

private:
    friend class Editor;
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<EmitterComponent>(entt::registry& reg, entt::registry::entity_type e);
}