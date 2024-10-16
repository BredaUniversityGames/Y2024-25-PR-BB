#pragma once
#include "imgui_entt_entity_editor.hpp"

struct WorldMatrixComponent
{
private:
    glm::mat4 _worldMatrix { 1.0f };
    friend class TransformHelpers;

public:
    void Inspect();
};

namespace MM
{
template <>
void ComponentEditorWidget<WorldMatrixComponent>(entt::registry& reg, entt::registry::entity_type e);
}