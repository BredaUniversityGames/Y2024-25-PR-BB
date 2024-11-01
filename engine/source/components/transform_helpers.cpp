#include "components/transform_helpers.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "components/transform_component.hpp"
#include "components/relationship_component.hpp"
#include "components/world_matrix_component.hpp"
#include <entt/entity/registry.hpp>

void TransformHelpers::SetLocalPosition(entt::registry& reg, entt::entity entity, const glm::vec3& position)
{
    assert(reg.valid(entity));
    TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    if (transform == nullptr || transform->_localPosition == position)
    {
        return;
    }

    transform->_localPosition = position;
    UpdateWorldMatrix(reg, entity);
}
void TransformHelpers::SetLocalRotation(entt::registry& reg, entt::entity entity, const glm::quat& rotation)
{
    assert(reg.valid(entity));
    TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    if (transform == nullptr || transform->_localRotation == rotation)
    {
        return;
    }

    transform->_localRotation = rotation;
    UpdateWorldMatrix(reg, entity);
}
void TransformHelpers::SetLocalScale(entt::registry& reg, entt::entity entity, const glm::vec3& scale)
{
    assert(reg.valid(entity));
    TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    if (transform == nullptr || transform->_localScale == scale)
    {
        return;
    }

    transform->_localScale = scale;
    UpdateWorldMatrix(reg, entity);
}
void TransformHelpers::SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
{
    TransformComponent* transform = reg.try_get<TransformComponent>(entity);

    if (!transform)
    {
        return;
    }

    transform->_localPosition = position;
    transform->_localRotation = rotation;
    transform->_localScale = scale;

    UpdateWorldMatrix(reg, entity);
}
void TransformHelpers::SetWorldTransform(entt::registry& reg, entt::entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
{
    assert(reg.valid(entity));
    TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    RelationshipComponent* relationship = reg.try_get<RelationshipComponent>(entity);

    if (!transform)
    {
        return;
    }

    if (relationship && relationship->parent != entt::null)
    {
        WorldMatrixComponent* parentWorldMatrix = reg.try_get<WorldMatrixComponent>(relationship->parent);

        glm::vec3 parentScale {}, skew {}, parentTranslation {};
        glm::quat parentOrientation {};
        glm::vec4 perspective {};

        glm::decompose(parentWorldMatrix->_worldMatrix, parentScale, parentOrientation, parentTranslation, skew, perspective);

        transform->_localPosition = glm::vec3 { glm::inverse(parentWorldMatrix->_worldMatrix) * glm::vec4 { transform->_localPosition, 1.0f } };
        transform->_localRotation = glm::inverse(parentOrientation) * rotation;
        transform->_localScale = transform->_localScale / parentScale;
    }
    else
    {
        transform->_localPosition = position;
        transform->_localRotation = rotation;
        transform->_localScale = scale;
    }

    UpdateWorldMatrix(reg, entity);
}
glm::vec3 TransformHelpers::GetLocalPosition(const entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    return transform == nullptr ? glm::vec3 {} : transform->_localPosition;
}
glm::quat TransformHelpers::GetLocalRotation(const entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    return transform == nullptr ? glm::quat { 1.0f, 0.0f, 0.0f, 0.0f } : transform->_localRotation;
}
glm::vec3 TransformHelpers::GetLocalScale(const entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    return transform == nullptr ? glm::vec3 { 1.0f, 1.0f, 1.0f } : transform->_localScale;
}
glm::mat4 TransformHelpers::GetLocalMatrix(const entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    return transform == nullptr ? glm::mat4 { 1.0f } : ToMatrix(transform->_localPosition, transform->_localRotation, transform->_localScale);
}
const glm::mat4& TransformHelpers::GetWorldMatrix(entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const WorldMatrixComponent& worldMatrix = reg.get_or_emplace<WorldMatrixComponent>(entity);

    return worldMatrix._worldMatrix;
}
glm::mat4 TransformHelpers::ToMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
{
    // TODO Can be optimized
    const glm::mat4 translationMatrix = glm::translate(glm::mat4 { 1.0f }, position);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4 { 1.0f }, scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}
void TransformHelpers::OnConstructTransform(entt::registry& reg, entt::entity entity)
{
    reg.emplace<WorldMatrixComponent>(entity);

    UpdateWorldMatrix(reg, entity);
}
void TransformHelpers::OnDestroyTransform(entt::registry& reg, entt::entity entity)
{
    reg.remove<WorldMatrixComponent>(entity);
}
void TransformHelpers::SubscribeToEvents(entt::registry& reg)
{
    reg.on_construct<TransformComponent>().connect<&OnConstructTransform>();
    reg.on_destroy<TransformComponent>().connect<&OnDestroyTransform>();
}
void TransformHelpers::UnsubscribeToEvents(entt::registry& reg)
{
    reg.on_construct<TransformComponent>().disconnect<&OnConstructTransform>();
    reg.on_destroy<TransformComponent>().disconnect<&OnDestroyTransform>();
}
void TransformHelpers::UpdateWorldMatrix(entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const RelationshipComponent* relationship = reg.try_get<RelationshipComponent>(entity);
    WorldMatrixComponent& worldMatrix = reg.get_or_emplace<WorldMatrixComponent>(entity);

    if (!relationship)
    {
        worldMatrix._worldMatrix = GetLocalMatrix(reg, entity);
        return;
    }

    if (relationship->parent == entt::null)
    {
        worldMatrix._worldMatrix = GetLocalMatrix(reg, entity);
    }
    else
    {
        // TODO optimize by sorting based on relationship components
        worldMatrix._worldMatrix = GetWorldMatrix(reg, relationship->parent) * GetLocalMatrix(reg, entity);
    }

    // Iterate over all children and update their world matrices
    entt::entity current = relationship->first;
    for (size_t i {}; i < relationship->childrenCount; ++i)
    {
        if (current != entt::null)
        {
            UpdateWorldMatrix(reg, current);

            current = reg.get<RelationshipComponent>(current).next;
        }
    }
}