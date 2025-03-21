﻿#pragma once
#include "common.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "entt/entity/entity.hpp"
#include "physics_module.hpp"

struct Vertex;
struct Hierarchy;
class PhysicsModule;
struct CPUModel;
template <typename T>
struct CPUMesh;

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
    // for loading mesh data or convex data into the scene with no rendering mesh relation
    entt::entity LoadNodeRecursive(const CPUModel& models, ECSModule& ecs, uint32_t currentNodeIndex, const Hierarchy& hierarchy, entt::entity parent, PhysicsShapes shape);

    // for loading mesh data or convex data into the scene. returns a vector of rigidbodies
    std::vector<RigidbodyComponent> LoadBodiesRecursive(const CPUModel& models, ECSModule& ecs, uint32_t currentNodeIndex, const Hierarchy& hierarchy, entt::entity parent, PhysicsShapes shape);

    Engine& engine;
    ECSModule& _ecs;
    PhysicsModule& _physicsModule;
};
