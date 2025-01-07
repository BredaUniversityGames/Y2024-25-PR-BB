#pragma once
#include "common.hpp"
#include "ecs_module.hpp"
#include "entt/entity/entity.hpp"
#include "physics_module.hpp"

struct Vertex;
struct Hierarchy;
class PhysicsModule;
struct RigidbodyComponent;
struct CPUModel;
template <typename T>
struct CPUMesh;
class ModelLoader;
class PhysicsSystem : public SystemInterface
{
public:
    PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule);
    ~PhysicsSystem();
    NON_COPYABLE(PhysicsSystem);
    NON_MOVABLE(PhysicsSystem);

    RigidbodyComponent CreateMeshColliderBody(const CPUMesh<Vertex>& mesh, PhysicsShapes shapeType, entt::entity entityToAttachTo = entt::null);

    void CreateCollision(const std::string& path, const PhysicsShapes shapeType);

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

    std::unique_ptr<ModelLoader> _collisionLoader;
    Engine& engine;
    ECSModule& _ecs;
    PhysicsModule& _physicsModule;
};