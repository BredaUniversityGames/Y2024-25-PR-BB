#pragma once
#include <vector>
#include <entity/entity.hpp>
#include <glm/glm.hpp>
#include <entity/component.hpp>

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

    bool HasChildren() const { return !_children.empty(); }

    const std::vector<std::reference_wrapper<TransformComponent>>& GetChildren() const;

    bool IsAForeFather(const TransformComponent& potentialForeFather) const;

    entt::entity GetOwner() const { return _owner; }

    static void SubscribeToEvents(entt::registry& registry);

    static void UnsubscribeFromEvents(entt::registry& registry);

    static glm::mat4 ToMatrix(glm::vec3 position, glm::quat rotation, glm::vec3 scale);

    glm::vec3 GetLocalPosition() const;
    glm::quat GetLocalRotation() const;
    glm::vec3 GetLocalScale() const;

    glm::vec3 GetWorldPosition() const;
    glm::quat GetWorldRotation() const;
    glm::vec3 GetWorldScale() const;

    void SetLocalPosition(glm::vec3 position);
    void SetLocalRotation(glm::quat rotation);
    void SetLocalScale(glm::vec3 scale);

    void SetWorldPosition(glm::vec3 position);
    void SetWorldRotation(glm::quat rotation);
    void SetWorldScale(glm::vec3 scale);

    const glm::mat4& GetWorldMatrix() const;
    glm::mat4 GetLocalMatrix() const;

    void SetWorldMatrix(const glm::mat4& mat);
    void SetLocalMatrix(const glm::mat4& mat);

private:
    void UpdateWorldMatrix();

    friend Editor;

    glm::mat4 _worldMatrix { 1.0f };

    glm::vec3 _localPosition {};
    glm::quat _localRotation { 1.0f, 0.f, 0.f, 0.f };
    glm::vec3 _localScale { 1.0f, 1.0f, 1.0f };

    entt::entity _owner = entt::null;
    TransformComponent* _parent {};
    std::vector<std::reference_wrapper<TransformComponent>> _children {};
};

// Enabling pointer stability in entt
template <>
struct entt::component_traits<TransformComponent, void>
{
    using type = TransformComponent;

    static constexpr bool in_place_delete = true;

    static constexpr std::size_t page_size = internal::page_size<TransformComponent>::value;
};
