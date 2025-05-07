#pragma once
#include "common.hpp"
#include <Jolt/Jolt.h>

#include <Jolt/Physics/Collision/ContactListener.h>

// An example contact listener
class PhysicsContactListener : public JPH::ContactListener
{
public:
    // See: ContactListener
    JPH::ValidateResult OnContactValidate(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED JPH::RVec3Arg inBaseOffset,
        MAYBE_UNUSED const JPH::CollideShapeResult& inCollisionResult) override
    {
        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

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
};