#pragma once
#include "common.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "entt/entity/entity.hpp"
#include "physics_module.hpp"

class PhysicsModule;

class PhysicsSystem : public SystemInterface
{
public:
    PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule);
    ~PhysicsSystem() = default;
    NON_COPYABLE(PhysicsSystem);
    NON_MOVABLE(PhysicsSystem);

    void Update(ECSModule& ecs, float deltaTime) override;
    void Render(const ECSModule& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

    entt::entity _playerEntity = entt::null;
    std::string_view GetName() override { return "PhysicsSystem"; }

private:
    Engine& engine;
    ECSModule& _ecs;
    PhysicsModule& _physicsModule;
};
