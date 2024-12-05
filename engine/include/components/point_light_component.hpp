#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>

struct PointLightComponent
{
    glm::vec3 color { 1.0f };
    float range = 10.0f;
    float attenuation = 0.5f;

    friend class TransformHelpers;
    friend class Editor;

    void Inspect();
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<PointLightComponent>(entt::registry& reg, entt::registry::entity_type e);
}