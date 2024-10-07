#pragma once
#include "modules/physics_module.hpp"

struct RigidbodyComponent
{
    RigidbodyComponent(BodyInterface& body_interface)
    {
        BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0, 2.0, 0.0), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
        bodyID = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
    }

    BodyID bodyID;
};