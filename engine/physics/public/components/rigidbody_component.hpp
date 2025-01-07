#pragma once
#include "physics_module.hpp"
#include "math_util.hpp"

struct UpdateMeshAndPhysics
{
};
struct RigidbodyComponent
{
    // default creates a sphere at 0,2,0
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, PhysicsShapes shape = eSPHERE, BodyType type = eDYNAMIC)
        : shapeType(shape)
        , bodyType(type)
    {
        JPH::BodyCreationSettings bodySettings;

        if (shape == eSPHERE)
        {
            if (bodyType == eDYNAMIC)
            {
                bodySettings = JPH::BodyCreationSettings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
            }
            else if (bodyType == eSTATIC)
            {
                bodySettings = JPH::BodyCreationSettings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
            }

            bodySettings.mAllowDynamicOrKinematic = true;
            bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
        }
        else if (shape == eBOX)
        {
            if (bodyType == eDYNAMIC)
            {
                bodySettings = JPH::BodyCreationSettings(new JPH::BoxShape(JPH::Vec3(0.5, 0.5, 0.5)), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
            }
            else if (bodyType == eSTATIC)
            {
                bodySettings = JPH::BodyCreationSettings(new JPH::BoxShape(JPH::Vec3(0.5, 0.5, 0.5)), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
            }
            bodySettings.mAllowDynamicOrKinematic = true;
            bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
        }

        // set the owner entity so we can query it later from physics objects if needed
        bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
    }

    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, JPH::BodyCreationSettings& bodyCreationSettings)
        : shapeType(eCUSTOM)
    {
        bodyID = bodyInterface.CreateAndAddBody(bodyCreationSettings, JPH::EActivation::Activate);

        // set the owner entity so we can querry it later from physics ohbejcts if needed
        bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
    }

    // for AABB collisions
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, math::Vec3Range boundingBox, BodyType type = eSTATIC)
        : shapeType(eBOX)
        , bodyType(type)
    {
        glm::vec3 halfExtents = (boundingBox.max - boundingBox.min) * 0.5f;
        JPH::BodyCreationSettings bodySettings;
        halfExtents = glm::abs(halfExtents);
        if (bodyType == eSTATIC)
        {
            bodySettings = JPH::BodyCreationSettings(
                new JPH::BoxShape(JPH::Vec3Arg(halfExtents.x, halfExtents.y, halfExtents.z), 0.01f),
                JPH::Vec3Arg(position.x, position.y, position.z),
                JPH::QuatArg::sIdentity(),
                JPH::EMotionType::Static,
                PhysicsLayers::NON_MOVING);
        }
        else if (bodyType == eDYNAMIC)
        {
            bodySettings = JPH::BodyCreationSettings(
                new JPH::BoxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z), 0.01f),
                JPH::Vec3(position.x, position.y, position.z),
                JPH::Quat::sIdentity(),
                JPH::EMotionType::Dynamic,
                PhysicsLayers::MOVING);
        }
        bodySettings.mAllowDynamicOrKinematic = true;
        bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

        // set the owner entity so we can query it later from physics objects if needed
        bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
    }

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
    BodyType bodyType;
};