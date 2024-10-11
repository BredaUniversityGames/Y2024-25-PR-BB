#pragma once

#include <entity/entity.hpp>

class TransformHelpers
{
public:
    static void SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
    static void SetWorldTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

    static glm::vec3 GetLocalPosition(const entt::registry& reg, entt::entity entity);
    static glm::quat GetLocalRotation(const entt::registry& reg, entt::entity entity);
    static glm::vec3 GetLocalScale(const entt::registry& reg, entt::entity entity);

    static glm::mat4 GetLocalMatrix(const entt::registry& reg, entt::entity entity);
    static const glm::mat4& GetWorldMatrix(entt::registry& reg, entt::entity entity);

    static glm::mat4 ToMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

private:
    friend class Editor;
    static void UpdateWorldMatrix(entt::registry& reg, entt::entity entity);
};