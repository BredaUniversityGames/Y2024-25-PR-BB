#include "components/animation_transform_component.hpp"

#include <glm/gtx/matrix_decompose.hpp>

void AnimationTransformHelpers::SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::mat4& transform)
{
    glm::vec3 scale, skew, translation;
    glm::quat orientation;
    glm::vec4 perspective;

    glm::decompose(transform, scale, orientation, translation, skew, perspective);

    SetLocalTransform(reg, entity, translation, orientation, scale);
}
void AnimationTransformHelpers::SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
{
    AnimationTransformComponent& transform = reg.get<AnimationTransformComponent>(entity);

    transform.position = position;
    transform.rotation = rotation;
    transform.scale = scale;
}

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AnimationTransformComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<AnimationTransformComponent>(e);
    ImGui::InputFloat3("Position##AnimationTransform", &comp.position[0]);
    ImGui::InputFloat4("Rotation##AnimationTransform", &comp.rotation[0]);
    ImGui::InputFloat3("Scale##AnimationTransform", &comp.scale[0]);
}
}
