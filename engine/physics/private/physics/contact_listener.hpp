#pragma once
#include "common.hpp"
#include "physics/collision.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <entt/entity/registry.hpp>

#include <mutex>

class PhysicsContactListener : public JPH::ContactListener
{
public:
    PhysicsContactListener(entt::registry& registry)
        : registry(registry)
    {
    }

    // See: ContactListener
    JPH::ValidateResult OnContactValidate(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED JPH::RVec3Arg inBaseOffset,
        MAYBE_UNUSED const JPH::CollideShapeResult& inCollisionResult) override;

    void OnContactAdded(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED const JPH::ContactManifold& inManifold,
        MAYBE_UNUSED JPH::ContactSettings& ioSettings) override;

    void OnContactPersisted(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED const JPH::ContactManifold& inManifold,
        MAYBE_UNUSED JPH::ContactSettings& ioSettings) override;

    void OnContactRemoved(MAYBE_UNUSED const JPH::SubShapeIDPair& inSubShapePair) override;

private:
    // contact callbacks are triggered asynchronously
    std::mutex callbackMutex {};
    entt::registry& registry;
};