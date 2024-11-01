﻿#pragma once
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
    void CleanUp();

    void Update([[maybe_unused]] ECS& ecs, [[maybe_unused]] float deltaTime) override;
    void Render([[maybe_unused]] const ECS& ecs) const override;
    void Inspect() override;

private:
    ECS& _ecs;
    PhysicsModule& _physicsModule;
};