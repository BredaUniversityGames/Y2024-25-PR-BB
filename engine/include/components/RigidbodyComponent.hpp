#pragma once
#include "modules/physics_module.hpp"

struct RigidbodyComponent
{
    RigidbodyComponent(JPH::BodyInterface& body_interface)
    {
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
        bodyID = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
        body_interface.SetLinearVelocity(bodyID, JPH::Vec3(0.0f, 0.0f, 0.0f));
        body_interface.SetPosition(bodyID, JPH::Vec3(0.0f, 2.0f, 0.0f), JPH::EActivation::Activate);
    }

    JPH::BodyID bodyID;
};