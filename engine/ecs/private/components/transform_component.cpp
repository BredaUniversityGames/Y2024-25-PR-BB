#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include <glm/gtx/euler_angles.hpp>

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
    changed |= ImGui::DragFloat3("Position##Transform", &_localPosition[0], 0.1f);

    // Rotation: now showing/storing Euler angles instead of quaternion
    changed |= ImGui::DragFloat3("Rotation##Transform", &_editorEulerAngles[0], 0.1f);

    changed |= ImGui::DragFloat3("Scale##Transform", &_localScale[0], 0.1f);

    // If the user changed anything, update the internal quaternion
    if (changed)
    {

        // Convert that matrix to a quaternion
        _localRotation = glm::quat { glm::radians(_editorEulerAngles) }; // glm::quat_cast(rotMat);

        // Then notify the rest of the engine to update the world matrix
        TransformHelpers::UpdateWorldMatrix(reg, entity);
    }
    else
    {
        _editorEulerAngles = glm::degrees(glm::eulerAngles(_localRotation));
    }
}