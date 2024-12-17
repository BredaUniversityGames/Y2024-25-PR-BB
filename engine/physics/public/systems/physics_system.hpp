#pragma once
#include "common.hpp"
#include "ecs_module.hpp"
#include "entt/entity/entity.hpp"

struct Hierarchy;
class PhysicsModule;
struct RigidbodyComponent;
class CPUModel;
enum PhysicsShapes;
class PhysicsSystem : public SystemInterface
{
public:
    PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule);
    NON_COPYABLE(PhysicsSystem);
    NON_MOVABLE(PhysicsSystem);

    void InitializePhysicsColliders();

    void CreateMeshCollision(const std::string& path);
    void CreateConvexHullCollision(const std::string& path);

    void CleanUp();
    void Update(ECSModule& ecs, float deltaTime) override;
    void Render(const ECSModule& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

private:
    entt::entity LoadNodeRecursive(const CPUModel& models, ECSModule& ecs, uint32_t currentNodeIndex, Hierarchy& hierarchy, entt::entity parent, PhysicsShapes shape);

    Engine& engine;
    ECSModule& _ecs;
    PhysicsModule& _physicsModule;
};