#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/vec4.hpp>

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

    changed |= ImGui::DragFloat3("Position##Transform", &_localPosition[0], 0.1f);

    glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(glm::quat(_localRotation)));

    changed |= ImGui::DragFloat3("Rotation##Transform (Euler)", &eulerAngles[0], 0.1f);

    if (changed)
    {
        _localRotation = glm::quat(glm::radians(eulerAngles));
    }

    changed |= ImGui::DragFloat3("Scale##Transform", &_localScale[0], 0.1f);

    if (changed)
    {
        TransformHelpers::UpdateWorldMatrix(reg, entity);
    }
}
