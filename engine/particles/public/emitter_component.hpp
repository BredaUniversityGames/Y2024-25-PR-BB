#pragma once

#include "imgui_entt_entity_editor.hpp"
#include "particle_util.hpp"
#include <entt/entity/registry.hpp>

struct ActiveEmitterTag
{
};

struct ParticleEmitterComponent
{
    bool emitOnce = true;
    float maxEmitDelay = 1.0f;
    float currentEmitDelay = 1.0f;
    Emitter emitter;
    void Inspect();

private:
    friend class Editor;
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<ParticleEmitterComponent>(entt::registry& reg, entt::registry::entity_type e);
}