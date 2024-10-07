#include "components/transform_component.hpp"

#include <entity/registry.hpp>
#include <entity/helper.hpp>
#include <gmock/internal/gmock-internal-utils.h>

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

    // TODO Update transform

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

    // TODO More transform things
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