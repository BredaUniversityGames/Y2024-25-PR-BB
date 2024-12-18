#include "components/transform_component.hpp"
#include "components/relationship_component.hpp"
#include <glm/gtx/matrix_decompose.hpp>

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
void TransformHelpers::SetLocalTransform(entt::registry& reg, entt::entity entity, const glm::mat4& transform)
{
    glm::vec3 scale, skew, translation;
    glm::quat orientation;
    glm::vec4 perspective;

    glm::decompose(transform, scale, orientation, translation, skew, perspective);

    if (std::abs(scale.x) < 0.0001f || std::abs(scale.y) < 0.0001f || std::abs(scale.z) < 0.0001f)
    {
        bblog::warn("Too small scale");
        return;
    }

    SetLocalTransform(reg, entity, translation, orientation, scale);
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
void TransformHelpers::SetWorldTransform(entt::registry& reg, entt::entity entity, const glm::mat4& worldMatrix)
{
    assert(reg.valid(entity));

    TransformComponent* transform = reg.try_get<TransformComponent>(entity);
    RelationshipComponent* relationship = reg.try_get<RelationshipComponent>(entity);

    if (!transform)
    {
        return;
    }

    glm::mat4 localMatrix = worldMatrix;

    if (relationship && relationship->parent != entt::null)
    {
        WorldMatrixComponent* parentWorldMatrixComponent = reg.try_get<WorldMatrixComponent>(relationship->parent);
        if (parentWorldMatrixComponent)
        {
            glm::mat4 parentWorldMatrix = parentWorldMatrixComponent->_worldMatrix;
            glm::mat4 inverseParentWorldMatrix = glm::inverse(parentWorldMatrix);
            localMatrix = inverseParentWorldMatrix * worldMatrix;
        }
    }

    // Decompose the localMatrix to obtain position, rotation, and scale
    glm::vec3 scale, translation, skew;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(localMatrix, scale, rotation, translation, skew, perspective);

    // Set the local transform components
    transform->_localPosition = translation;
    transform->_localRotation = rotation;
    transform->_localScale = scale;

    // Update the world matrix of the entity
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

glm::vec3 TransformHelpers::GetLocalPosition(const TransformComponent& transformComponent)
{
    return transformComponent._localPosition;
}

glm::quat TransformHelpers::GetLocalRotation(const TransformComponent& transformComponent)
{
    return transformComponent._localRotation;
}

glm::vec3 TransformHelpers::GetLocalScale(const TransformComponent& transformComponent)
{
    return transformComponent._localScale;
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
const glm::mat4& TransformHelpers::GetWorldMatrix(const entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    const WorldMatrixComponent& worldMatrix = reg.get<WorldMatrixComponent>(entity);

    return worldMatrix._worldMatrix;
}
const glm::mat4& TransformHelpers::GetWorldMatrix(const WorldMatrixComponent& worldMatrixComponent)
{
    return worldMatrixComponent._worldMatrix;
}

glm::vec3 TransformHelpers::GetWorldPosition(entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    auto& m = TransformHelpers::GetWorldMatrix(reg, entity);

    return glm::vec3(m[3][0], m[3][1], m[3][2]);
}
glm::quat TransformHelpers::GetWorldRotation(entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    auto& m = TransformHelpers::GetWorldMatrix(reg, entity);

    return glm::quat_cast(m);
}
glm::vec3 TransformHelpers::GetWorldScale(entt::registry& reg, entt::entity entity)
{
    assert(reg.valid(entity));
    auto& m = TransformHelpers::GetWorldMatrix(reg, entity);

    glm::vec3 col0 = glm::vec3(m[0][0], m[0][1], m[0][2]);
    glm::vec3 col1 = glm::vec3(m[1][0], m[1][1], m[1][2]);
    glm::vec3 col2 = glm::vec3(m[2][0], m[2][1], m[2][2]);

    return glm::vec3(glm::length(col0), glm::length(col1), glm::length(col2));
}
glm::mat4 TransformHelpers::ToMatrix(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
{
    // TODO Can be optimized
    const glm::mat4 translationMatrix = glm::translate(glm::mat4 { 1.0f }, position);
    const glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
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