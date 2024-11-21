#pragma once
#include "entt/entity/entity.hpp"
#include "systems/system.hpp"

struct RayHitInfo
{
    entt::entity entity = entt::null; // Entity that was hit
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); // Position where the ray hits; HitPoint = Start + mFraction * (End - Start)
    float hitFraction = 0.0f; // Hit fraction of the ray/object [0, 1], HitPoint = Start + mFraction * (End - Start)
    bool hasHit = false;
};

class PhysicsModule;
struct RigidbodyComponent;
class PhysicsSystem : public System
{
public:
    PhysicsSystem(ECS& ecs, PhysicsModule& physicsModule);
    ~PhysicsSystem() = default;

    void AddRigidBody(entt::entity entity, RigidbodyComponent& rigidbody);
    [[nodiscard]] RayHitInfo ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const;
    void CleanUp();

    void Update([[maybe_unused]] ECS& ecs, [[maybe_unused]] float deltaTime) override;
    void Render([[maybe_unused]] const ECS& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

    void SetCameraPosition(const glm::vec3& position) { _cameraPosition = position; }
    void SetCameraDirection(const glm::vec3& direction) { _cameraDirection = direction; }

private:
    ECS& _ecs;
    PhysicsModule& _physicsModule;
    glm::vec3 _cameraPosition;
    glm::vec3 _cameraDirection;
};