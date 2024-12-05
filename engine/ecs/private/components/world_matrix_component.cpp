#include "components/world_matrix_component.hpp"
#include <imgui.h>

void WorldMatrixComponent::Inspect()
{
    ImGui::DragFloat4("col 1##WorldMatrix", &_worldMatrix[0].x, 0.1f);
    ImGui::DragFloat4("col 2##WorldMatrix", &_worldMatrix[1].x, 0.1f);
    ImGui::DragFloat4("col 3##WorldMatrix", &_worldMatrix[2].x, 0.1f);
    ImGui::DragFloat4("col 4##WorldMatrix", &_worldMatrix[3].x, 0.1f);
}

namespace EnttEditor
{
template <>
void ComponentEditorWidget<WorldMatrixComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<WorldMatrixComponent>(e);
    comp.Inspect();
}
}