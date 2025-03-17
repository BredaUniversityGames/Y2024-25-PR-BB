#pragma once

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

struct AnimationTransformComponent
{
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

namespace AnimationTransformHelpers
{
void SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::mat4& transform);
void SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
}

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AnimationTransformComponent>(entt::registry& reg, entt::registry::entity_type e);
}
