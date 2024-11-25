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

    void AddRigidBody(entt::entity entity, RigidbodyComponent& rigidbody);
    NO_DISCARD RayHitInfo ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const;
    void CleanUp();

    void Update(MAYBE_UNUSED ECS& ecs, MAYBE_UNUSED float deltaTime) override;
    void Render(MAYBE_UNUSED const ECS& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

private:
    ECS& _ecs;
    PhysicsModule& _physicsModule;
};