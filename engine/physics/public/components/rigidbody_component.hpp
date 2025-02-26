#pragma once

#include <entt/entity/registry.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

struct UpdateMeshAndPhysics
{
};

class RigidbodyComponent
{
public:
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::ShapeRefC shape, bool dynamic, JPH::EAllowedDOFs freedom = JPH::EAllowedDOFs::All);

    static void SetupRegistryCallbacks(entt::registry& registry);
    static void DisconnectRegistryCallbacks(entt::registry& registry);

    JPH::BodyID bodyID;
    JPH::ShapeRefC shape;

private:
    bool dynamic = false;
    JPH::EAllowedDOFs dofs {};
    JPH::BodyInterface& bodyInterface;

    static void OnDestroyCallback(entt::registry& registry, entt::entity entity);
    static void OnConstructCallback(entt::registry& registry, entt::entity entity);
};