﻿#pragma once
#include "modules/physics_module.hpp"

struct RigidbodyComponent
{
    // default creates a sphere at 0,2,0
    RigidbodyComponent(JPH::BodyInterface& body_interface)
    {
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
        bodyID = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    }

    RigidbodyComponent(JPH::BodyInterface& body_interface, JPH::BodyCreationSettings& body_creation_settings)
    {
        bodyID = body_interface.CreateAndAddBody(body_creation_settings, JPH::EActivation::Activate);
    }

    JPH::BodyID bodyID;
};