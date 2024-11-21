#pragma once
#include "physics_module.hpp"

struct RigidbodyComponent
{
    // default creates a sphere at 0,2,0
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, PhysicsShapes shape = eSPHERE, entt::entity ownerEntity = entt::null)
    {
        shapeType = shape;
        if (shape == eSPHERE)
        {
            JPH::BodyCreationSettings const bodySettings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
            bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
        }
        else if (shape == eBOX)
        {
            JPH::BodyCreationSettings const bodySettings(new JPH::BoxShape(JPH::Vec3(0.5, 0.5, 0.5)), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
            bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
        }

        // set the owner entity so we can query it later from physics objects if needed
        bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
    }

    RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::BodyCreationSettings& bodyCreationSettings, entt::entity ownerEntity)
    {
        bodyID = bodyInterface.CreateAndAddBody(bodyCreationSettings, JPH::EActivation::Activate);

        // set the owner entity so we can querry it later from physics ohbejcts if needed
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
};