#pragma once
#include "entt/entity/entity.hpp"
#include "systems/system.hpp"

class PhysicsModule;
struct RigidbodyComponent;
class PhysicsSystem : public System
{
public:
    PhysicsSystem(ECS& ecs, PhysicsModule& physicsModule);

    void CleanUp();
    void Update(MAYBE_UNUSED ECS& ecs, MAYBE_UNUSED float deltaTime) override;
    void Render(MAYBE_UNUSED const ECS& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

private:
    ECS& _ecs;
    PhysicsModule& _physicsModule;
};