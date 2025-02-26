#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <glm/gtc/constants.hpp>

struct CameraComponent
{
    enum class Projection : uint8_t
    {
        ePerspective,
        eOrthographic,

        eCount
    };

    Projection projection = Projection::ePerspective;
    float fov = glm::half_pi<float>();
    float nearPlane = 0.01f;
    float farPlane = 600.0f;

    float orthographicSize {};
    float aspectRatio {};

    bool reversedZ { false };

    friend class TransformHelpers;
    friend class Editor;

    void Inspect();
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<CameraComponent>(entt::registry& reg, entt::registry::entity_type e);
}