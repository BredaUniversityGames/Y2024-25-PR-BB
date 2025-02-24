#pragma once
#include "math_util.hpp"
#include "physics/collision.hpp"
#include <entt/entity/entity.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

struct UpdateMeshAndPhysics
{
};

struct RigidbodyComponent
{
    // default creates a sphere at 0,2,0
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, PhysicsShapes shapeType = PhysicsShapes::eSPHERE, BodyType type = BodyType::eDYNAMIC);

    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, JPH::BodyCreationSettings& bodyCreationSettings);

    // for mesh collisions
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, JPH::VertexList& vertices, JPH::IndexedTriangleList& triangles);

    // for convex collisions
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, JPH::VertexList& vertices);

    // for AABB collisions
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, math::Vec3Range boundingBox, BodyType type = BodyType::eSTATIC);

    void SetOwnerEntity(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity)
    {
        bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
    }

    entt::entity GetOwnerEntity(JPH::BodyInterface& bodyInterface)
    {
        return static_cast<entt::entity>(bodyInterface.GetUserData(bodyID));
    }

    RigidbodyComponent() = default;

    JPH::BodyID bodyID;
    PhysicsShapes shapeType;
    JPH::RefConst<JPH::Shape> shape;
    BodyType bodyType;
};