#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent
{
    glm::vec3 GetLocalPosition() const { return _localPosition; }
    glm::vec3 GetLocalScale() const { return _localScale; }
    glm::quat GetLocalRotation() const { return _localRotation; }
    void Inspect(entt::registry& reg, entt::entity entity);

private:
    glm::vec3 _localPosition {};
    glm::quat _localRotation { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 _localScale { 1.0f, 1.0f, 1.0f };

    // New: Store editor angles in degrees so the user can
    // freely edit them without quaternion → Euler back-conversion:
    glm::vec3 _editorEulerAngles { 0.0f, 0.0f, 0.0f };

    friend class TransformHelpers;
    friend class Editor;
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<TransformComponent>(entt::registry& reg, entt::registry::entity_type e);
}