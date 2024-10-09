#include "components/transform_component.hpp"

#include <entity/registry.hpp>
#include <glm/glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

TransformComponent::TransformComponent(TransformComponent& other) noexcept
    : _localPosition(other._localPosition)
    , _localRotation(other._localRotation)
    , _localScale(other._localScale)
    , _owner(other._owner)
{
    SetParent(other._parent);
}
TransformComponent::TransformComponent(TransformComponent&& other) noexcept
    : _owner(other._owner)
{
    _localPosition = other._localPosition;
    _localRotation = other._localRotation;
    _localScale = other._localScale;

    SetParent(other._parent);
    other.SetParent(nullptr);
}
TransformComponent::~TransformComponent()
{
    const std::vector<std::reference_wrapper<TransformComponent>> copyChildren = _children;

    for (TransformComponent& child : copyChildren)
    {
        child.SetParent(nullptr);
    }
    SetParent(nullptr);
}
void TransformComponent::OnCreate(entt::registry& registry, entt::entity entity)
{
    registry.get<TransformComponent>(entity)._owner = entity;
}
void TransformComponent::SetParent(TransformComponent* parent)
{
    if (_parent == parent
        || parent == this)
    {
        return;
    }

    if (parent != nullptr && parent->IsAForeFather(*this))
    {
        parent->SetParent(nullptr);
    }

    if (_parent != nullptr)
    {
        const auto it = std::find_if(_parent->_children.begin(), _parent->_children.end(),
            [&](const TransformComponent& transform)
            {
                return this == &transform;
            });
        assert(it != _parent->_children.end());

        _parent->_children.erase(it);
    }

    _parent = parent;

    if (_parent != nullptr)
    {
        parent->_children.emplace_back(*this);
    }

    UpdateWorldMatrix();
}
bool TransformComponent::IsOrphan() const
{
    return _parent == nullptr;
}
const std::vector<std::reference_wrapper<TransformComponent>>& TransformComponent::GetChildren() const
{
    return _children;
}
bool TransformComponent::IsAForeFather(const TransformComponent& potentialForeFather) const
{
    if (_parent == nullptr)
    {
        return false;
    }
    if (_parent == &potentialForeFather)
    {
        return true;
    }
    return _parent->IsAForeFather(potentialForeFather);
}
void TransformComponent::SubscribeToEvents(entt::registry& registry)
{
    registry.on_construct<TransformComponent>().connect<&TransformComponent::OnCreate>();
}
void TransformComponent::UnsubscribeFromEvents(entt::registry& registry)
{
    registry.on_construct<TransformComponent>().disconnect<&TransformComponent::OnCreate>();
}
glm::mat4 TransformComponent::ToMatrix(const glm::vec3 position, const glm::quat rotation, const glm::vec3 scale)
{
    const glm::mat4 translationMatrix = glm::translate(glm::mat4 { 1.0f }, position);
    const glm::mat4 rotationMatrix = glm::toMat4(rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4 { 1.0f }, scale);

    return translationMatrix * rotationMatrix * scaleMatrix;
}
glm::vec3 TransformComponent::GetLocalPosition() const
{
    return _localPosition;
}
glm::quat TransformComponent::GetLocalRotation() const
{
    return _localRotation;
}
glm::vec3 TransformComponent::GetLocalScale() const
{
    return _localScale;
}
glm::vec3 TransformComponent::GetWorldPosition() const
{
    return _parent == nullptr ? _localPosition : _worldMatrix * glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f };
}
glm::quat TransformComponent::GetWorldRotation() const
{
    return _parent == nullptr ? _localRotation : glm::quat_cast(_worldMatrix);
}
glm::vec3 TransformComponent::GetWorldScale() const
{
    if (_parent == nullptr)
    {
        return _localScale;
    }

    glm::vec3 scale, translation, skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(_worldMatrix, scale, rotation, translation, skew, perspective);

    return scale;
}
void TransformComponent::SetLocalPosition(const glm::vec3 position)
{
    _localRotation = position;
    UpdateWorldMatrix();
}
void TransformComponent::SetLocalRotation(const glm::quat rotation)
{
    _localRotation = rotation;
    UpdateWorldMatrix();
}
void TransformComponent::SetLocalScale(const glm::vec3 scale)
{
    _localScale = scale;
    UpdateWorldMatrix();
}
void TransformComponent::SetWorldPosition(glm::vec3 position)
{
    if (_parent == nullptr)
    {
        SetLocalPosition(position);
    }
    else
    {
        const glm::mat4 parentInverseMatrix = glm::inverse(_parent->GetWorldMatrix());
        SetLocalPosition(glm::vec3 { parentInverseMatrix * glm::vec4 { position, 1.0f } });
    }
}
void TransformComponent::SetWorldRotation(glm::quat rotation)
{
    if (_parent == nullptr)
    {
        SetLocalRotation(rotation);
    }
    else
    {
        const glm::quat parentRotation = _parent->GetWorldRotation();
        SetLocalRotation(glm::inverse(parentRotation) * rotation);
    }
}
void TransformComponent::SetWorldScale(glm::vec3 scale)
{
    if (_parent == nullptr)
    {
        SetLocalScale(scale);
    }
    else
    {
        const glm::vec3 parentScale = _parent->GetWorldScale();
        SetLocalRotation(scale / parentScale);
    }
}
const glm::mat4& TransformComponent::GetWorldMatrix() const
{
    return _worldMatrix;
}
glm::mat4 TransformComponent::GetLocalMatrix() const
{
    return ToMatrix(_localPosition, _localRotation, _localScale);
}
void TransformComponent::SetWorldMatrix(const glm::mat4& mat)
{
    glm::vec3 translation, scale, skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(mat, scale, rotation, translation, skew, perspective);

    SetWorldPosition(translation);
    SetWorldRotation(rotation);
    SetWorldScale(scale);
}
void TransformComponent::SetLocalMatrix(const glm::mat4& mat)
{
    glm::vec3 translation, scale, skew;
    glm::vec4 perspective;
    glm::quat rotation;

    glm::decompose(mat, scale, rotation, translation, skew, perspective);

    _localPosition = translation;
    _localRotation = rotation;
    _localScale = scale;
}

void TransformComponent::UpdateWorldMatrix()
{
    if (_parent == nullptr)
    {
        _worldMatrix = GetLocalMatrix();
    }
    else
    {
        _worldMatrix = _parent->GetWorldMatrix() * GetLocalMatrix();
    }

    for (TransformComponent& child : _children)
    {
        child.UpdateWorldMatrix();
    }
}