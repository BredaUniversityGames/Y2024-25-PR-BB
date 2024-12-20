﻿#pragma once
#include "common.hpp"
#include "ecs_module.hpp"
#include "entt/entity/entity.hpp"

class PhysicsModule;
struct RigidbodyComponent;

class PhysicsSystem : public SystemInterface
{
public:
    PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule);
    NON_COPYABLE(PhysicsSystem);
    NON_MOVABLE(PhysicsSystem);

    void InitializePhysicsColliders();

    void CleanUp();
    void Update(ECSModule& ecs, float deltaTime) override;
    void Render(const ECSModule& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

private:
    Engine& engine;
    ECSModule& _ecs;
    PhysicsModule& _physicsModule;
};