#pragma once
#include "math_util.hpp"
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

// class RigidbodyComponent
// {
// public:
//     RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::ShapeRefC shape, bool dynamic)
//         : _bodyInterface(bodyInterface)
//         , _shape(shape)
//         , dynamic(dynamic)
//     {
//     }
//
//     ~RigidbodyComponent()
//     {
//         _bodyInterface.DestroyBody(_bodyID);
//     }
//
//     static void SetupRegistryCallbacks(entt::registry& registry)
//     {
//         registry.on_construct<RigidbodyComponent>().connect<OnConstructCallback>();
//     }
//
//     static void DisconnectRegistryCallbacks(entt::registry& registry)
//     {
//         registry.on_construct<RigidbodyComponent>().disconnect<OnConstructCallback>();
//     }
//
//     static void OnConstructCallback(entt::registry& registry, entt::entity entity)
//     {
//         auto& rb = registry.get<RigidbodyComponent>(entity);
//
//         JPH::EMotionType motionType = rb.dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
//         JPH::ObjectLayer layer = rb.dynamic ? eMOVING_OBJECT : eNON_MOVING_OBJECT;
//
//         auto scaledShape = rb._shape.GetPtr()->ScaleShape(
//             ToJoltVec3(TransformHelpers::GetWorldScale(registry, entity)));
//
//         rb._shape = scaledShape.Get();
//
//         JPH::BodyCreationSettings creation { scaledShape.Get(),
//             ToJoltVec3(TransformHelpers::GetWorldPosition(registry, entity)),
//             ToJoltQuat(TransformHelpers::GetWorldRotation(registry, entity)),
//             motionType, layer };
//
//         // Needed if we change from a static object to dynamic
//         creation.mAllowDynamicOrKinematic = true;
//
//         // Look into mass settings
//
//         JPH::EActivation activation = rb.dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
//
//         rb._bodyID = rb._bodyInterface.CreateAndAddBody(creation, activation);
//         rb._bodyInterface.SetUserData(rb._bodyID, static_cast<uint64_t>(entity));
//     }
//
// private:
//     JPH::BodyInterface& _bodyInterface;
//     JPH::BodyID _bodyID;
//     JPH::ShapeRefC _shape;
//     bool dynamic = false;
// };
//
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
    JPH::ShapeRefC shape;
    BodyType bodyType;
};