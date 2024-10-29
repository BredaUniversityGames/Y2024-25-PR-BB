#pragma once
#include "modules/physics_module.hpp"

struct RigidbodyComponent
{
    // default creates a sphere at 0,2,0
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, PhysicsShapes shape = eSPHERE)
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
    }

    RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::BodyCreationSettings& bodyCreationSettings)
    {
        bodyID = bodyInterface.CreateAndAddBody(bodyCreationSettings, JPH::EActivation::Activate);
    }

    RigidbodyComponent() = default;

    JPH::BodyID bodyID;
    PhysicsShapes shapeType;
};