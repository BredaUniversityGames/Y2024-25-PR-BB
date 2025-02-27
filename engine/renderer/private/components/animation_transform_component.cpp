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
