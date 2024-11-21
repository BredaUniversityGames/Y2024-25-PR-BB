#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>

struct DirectionalLightComponent
{
    glm::vec3 color { 1.0f };

    float shadowBias = 0.002f;
    float orthographicSize = 17.0f;
    float nearPlane = -16.0f;
    float farPlane = 32.0f;
    float aspectRatio = 1.0f;

    constexpr static glm::mat4 BIAS_MATRIX {
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0
    };

    friend class TransformHelpers;
    friend class Editor;

    void Inspect();
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<DirectionalLightComponent>(entt::registry& reg, entt::registry::entity_type e);
}