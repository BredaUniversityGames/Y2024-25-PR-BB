#pragma once
#include <entity/entity.hpp>

class Editor;
class TransformComponent
{
public:
    TransformComponent() = default;

    TransformComponent(TransformComponent& other) noexcept;
    TransformComponent(TransformComponent&& other) noexcept;

    TransformComponent& operator=(const TransformComponent&) = delete;
    TransformComponent& operator=(TransformComponent&&) = delete;

    ~TransformComponent();

    static void OnCreate(entt::registry& registry, entt::entity entity);

    void SetParent(TransformComponent* parent);

    bool IsOrphan() const;

    bool HasChildren() const { return _children.size() > 0; }

    const std::vector<std::reference_wrapper<TransformComponent>>& GetChildren() const;

    bool IsAForeFather(const TransformComponent& potentialForeFather) const;

    entt::entity GetOwner() { return _owner; }

    static void SubscribeToEvents(entt::registry& registry);

    static void UnsubscribeFromEvents(entt::registry& registry);

private:
    friend Editor;

    glm::vec3 _localPosition {};
    glm::quat _localRotation { 1.0f, 0.f, 0.f, 0.f };
    glm::vec3 _localScale { 1.0f, 1.0f, 1.0f };

    entt::entity _owner = entt::null;
    TransformComponent* _parent {};
    std::vector<std::reference_wrapper<TransformComponent>> _children {};
};