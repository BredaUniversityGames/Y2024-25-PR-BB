#pragma once

#include "imgui_entt_entity_editor.hpp"
#include "particle_util.hpp"
#include <entt/entity/registry.hpp>

struct ActiveEmitterTag
{
};

struct ParticleBurst
{
    float startTime = 0.0f;
    uint32_t count = 0;
    uint32_t cycles = 0;
    float maxInterval = 0.0f;
    float currentInterval = 0.0f;
    bool loop = false;
};

struct ParticleEmitterComponent
{
    bool emitOnce = true;
    uint32_t count = 0;
    float maxEmitDelay = 1.0f;
    float currentEmitDelay = 1.0f;
    glm::vec3 positionOffset = glm::vec3(0.0f);
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