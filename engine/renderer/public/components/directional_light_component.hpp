#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>

struct DirectionalLightComponent
{
    glm::vec3 color { 1.0f };

    float shadowBias = 0.002f;
    float poissonWorldOffset = 110.0f;
    float poissonConstant = 2048.0f; // Good results when we keep this the same size as the shadowmap
    float orthographicSize = 75.0f;
    float nearPlane = 0.1f;
    float farPlane = 200.0f;
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