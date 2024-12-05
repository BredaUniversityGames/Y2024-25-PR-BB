#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<TransformComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& t = reg.get<TransformComponent>(e);
    t.Inspect(reg, e);
}
}
void TransformComponent::Inspect(entt::registry& reg, entt::entity entity)
{
    bool changed = false;
    // TODO use euler angles for rotation
    changed |= ImGui::DragFloat3("Position##Transform", &_localPosition.x, 0.1f);
    changed |= ImGui::DragFloat4("Rotation##Transform", &_localRotation.x, 0.1f);
    changed |= ImGui::DragFloat3("Scale##Transform", &_localScale.x, 0.1f);

    if (changed)
    {
        TransformHelpers::UpdateWorldMatrix(reg, entity);
    }
}