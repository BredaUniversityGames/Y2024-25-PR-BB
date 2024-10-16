#pragma once
#include <entt/entity/registry.hpp>
#include "imgui_entt_entity_editor.hpp"
#include <glm/glm.hpp>

struct TransformComponent
{
private:
    glm::vec3 _localPosition {};
    glm::quat _localRotation = { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 _localScale = { 1.0f, 1.0f, 1.0f };
    friend class TransformHelpers;
    friend class Editor;

public:
    void Inspect(entt::registry& reg, entt::entity entity);
};

namespace MM
{
template <>
void ComponentEditorWidget<TransformComponent>(entt::registry& reg, entt::registry::entity_type e);
}