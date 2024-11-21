#pragma once

#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct WorldMatrixComponent;

class TransformHelpers
{
public:
    static void SetLocalPosition(entt::registry& reg, entt::entity entity, const glm::vec3& position);
    static void SetLocalRotation(entt::registry& reg, entt::entity entity, const glm::quat& rotation);
    static void SetLocalScale(entt::registry& reg, entt::entity entity, const glm::vec3& scale);

    // todo: matrix overload for these functions
    static void SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
    static void SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::mat4& transform);

    static void SetWorldTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

    static glm::vec3 GetLocalPosition(const entt::registry& reg, entt::entity entity);
    static glm::quat GetLocalRotation(const entt::registry& reg, entt::entity entity);
    static glm::vec3 GetLocalScale(const entt::registry& reg, entt::entity entity);

    static glm::mat4 GetLocalMatrix(const entt::registry& reg, entt::entity entity);
    static const glm::mat4& GetWorldMatrix(entt::registry& reg, entt::entity entity);
    static const glm::mat4& GetWorldMatrix(const WorldMatrixComponent& worldMatrixComponent);

    static glm::mat4 ToMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

    static void OnConstructTransform(entt::registry& reg, entt::entity entity);
    static void OnDestroyTransform(entt::registry& reg, entt::entity entity);

    static void SubscribeToEvents(entt::registry& reg);
    static void UnsubscribeToEvents(entt::registry& reg);

private:
    friend class Editor;
    friend struct TransformComponent;
    static void UpdateWorldMatrix(entt::registry& reg, entt::entity entity);
};