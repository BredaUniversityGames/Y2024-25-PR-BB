#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>

struct DirectionalLightComponent
{
    glm::vec3 color { 1.0f };

    float shadowBias = 0.0014f;
    float poissonWorldOffset = 4096.0f;
    float poissonConstant = 1024.0f; // Good results when we keep this the same size as the shadowmap
    float orthographicSize = 120.0f;
    float nearPlane = -50.0f;
    float farPlane = 500.0f;
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