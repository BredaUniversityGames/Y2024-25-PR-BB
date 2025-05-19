#pragma once

#include "emitter_preset_settings.hpp"
#include "imgui_entt_entity_editor.hpp"
#include "particle_util.hpp"
#include <entt/entity/registry.hpp>

enum class EmitterPresetID : uint8_t;

struct ActiveEmitterTag
{
};

// Only given to emitters that can be live edited via their presets.
struct TestEmitterTag
{
};

struct ParticleEmitterComponent
{
    bool emitOnce = true;
    uint32_t count = 0;
    float maxEmitDelay = 1.0f;
    float currentEmitDelay = 0.0f;
    glm::vec3 positionOffset = glm::vec3(0.0f);
    EmitterPresetID presetID;
    Emitter emitter;
    std::list<ParticleBurst> bursts = {};
    void Inspect();

private:
    friend class Editor;
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<ParticleEmitterComponent>(entt::registry& reg, entt::registry::entity_type e);
}