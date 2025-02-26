#pragma once
#include "physics/collision.hpp"
#include <components/transform_helpers.hpp>
#include <entt/entity/registry.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <physics/jolt_to_glm.hpp>

struct UpdateMeshAndPhysics
{
};

class RigidbodyComponent
{
public:
    RigidbodyComponent(JPH::BodyInterface& bodyInterface,
        JPH::ShapeRefC shape,
        bool dynamic,
        JPH::EAllowedDOFs freedom = JPH::EAllowedDOFs::All)
        : bodyInterface(bodyInterface)
        , shape(shape)
        , dynamic(dynamic)
        , dofs(freedom)
    {
    }

    static void SetupRegistryCallbacks(entt::registry& registry)
    {
        registry.on_construct<RigidbodyComponent>().connect<OnConstructCallback>();
    }

    static void DisconnectRegistryCallbacks(entt::registry& registry)
    {
        registry.on_construct<RigidbodyComponent>().disconnect<OnConstructCallback>();
    }

    static void OnConstructCallback(entt::registry& registry, entt::entity entity);

public:
    JPH::BodyInterface& bodyInterface;
    JPH::BodyID bodyID;
    JPH::ShapeRefC shape;
    bool dynamic = false;
    JPH::EAllowedDOFs dofs {};
};

// struct RigidbodyComponent
// {
//     // default creates a sphere at 0,2,0
//     RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, PhysicsShapes shapeType = PhysicsShapes::eSPHERE, BodyType type = BodyType::eDYNAMIC);
//
//     RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, JPH::BodyCreationSettings& bodyCreationSettings);
//
//     // for mesh collisions
//     RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, JPH::VertexList& vertices, JPH::IndexedTriangleList& triangles);
//
//     // for convex collisions
//     RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, JPH::VertexList& vertices);
//
//     // for AABB collisions
//     RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, math::Vec3Range boundingBox, BodyType type = BodyType::eSTATIC);
//
//     void SetOwnerEntity(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity)
//     {
//         bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
//     }
//
//     entt::entity GetOwnerEntity(JPH::BodyInterface& bodyInterface)
//     {
//         return static_cast<entt::entity>(bodyInterface.GetUserData(bodyID));
//     }
//
//     RigidbodyComponent() = default;
//
//     JPH::BodyID bodyID;
//     PhysicsShapes shapeType;
//     JPH::ShapeRefC shape;
//     BodyType bodyType;
// };