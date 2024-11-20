#pragma once
#include "systems/system.hpp"
#include "entt/entity/entity.hpp"

class PhysicsModule;
struct RigidbodyComponent;
class PhysicsSystem : public System
{
public:
    PhysicsSystem(ECS& ecs, PhysicsModule& physicsModule);
    ~PhysicsSystem() = default;

    entt::entity CreatePhysicsEntity();
    void CreatePhysicsEntity(RigidbodyComponent& rb);
    void AddRigidBody(entt::entity entity, RigidbodyComponent& rigidbody);
    void ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance);
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