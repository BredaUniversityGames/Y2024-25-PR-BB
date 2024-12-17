#pragma once
#include "common.hpp"
#include "ecs_module.hpp"
#include "entt/entity/entity.hpp"

struct Vertex;
struct Hierarchy;
class PhysicsModule;
struct RigidbodyComponent;
struct CPUModel;

template <typename T>
struct CPUMesh;

enum PhysicsShapes;
class PhysicsSystem : public SystemInterface
{
public:
    PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule);
    NON_COPYABLE(PhysicsSystem);
    NON_MOVABLE(PhysicsSystem);

    void InitializePhysicsColliders();

    void CreateMeshCollision(const std::string& path);
    RigidbodyComponent CreateMeshColliderBody(const CPUMesh<Vertex>& mesh, PhysicsShapes shapeType, entt::entity entityToAttachTo = entt::null);

    void CreateConvexHullCollision(const std::string& path);

    void CleanUp();
    void Update(ECSModule& ecs, float deltaTime) override;
    void Render(const ECSModule& ecs) const override;
    void Inspect() override;
    void InspectRigidBody(RigidbodyComponent& rb);

private:
    // for loading mesh data or convex data into the scene with no rendering mesh relation
    entt::entity LoadNodeRecursive(const CPUModel& models, ECSModule& ecs, uint32_t currentNodeIndex, const Hierarchy& hierarchy, entt::entity parent, PhysicsShapes shape);

    // for loading mesh data or convex data into the scene. returns a vector of rigidbodies
    std::vector<RigidbodyComponent> LoadBodiesRecursive(const CPUModel& models, ECSModule& ecs, uint32_t currentNodeIndex, const Hierarchy& hierarchy, entt::entity parent, PhysicsShapes shape);

    Engine& engine;
    ECSModule& _ecs;
    PhysicsModule& _physicsModule;
};