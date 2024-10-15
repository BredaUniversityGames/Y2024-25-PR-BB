#pragma once
#include "modules/physics_module.hpp"

struct RigidbodyComponent
{
    // default creates a sphere at 0,2,0
    RigidbodyComponent(JPH::BodyInterface& bodyInterface)
    {
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
        bodyID = bodyInterface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    }

    RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::BodyCreationSettings& bodyCreationSettings)
    {
        bodyID = bodyInterface.CreateAndAddBody(bodyCreationSettings, JPH::EActivation::Activate);
    }

    JPH::BodyID bodyID;
};