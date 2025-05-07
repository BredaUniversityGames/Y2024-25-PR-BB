#include "physics/contact_listener.hpp"

void PhysicsContactListener::OnContactAdded(
    MAYBE_UNUSED const JPH::Body& inBody1,
    MAYBE_UNUSED const JPH::Body& inBody2,
    MAYBE_UNUSED const JPH::ContactManifold& inManifold,
    MAYBE_UNUSED JPH::ContactSettings& ioSettings)
{
}

void PhysicsContactListener::OnContactPersisted(
    MAYBE_UNUSED const JPH::Body& inBody1,
    MAYBE_UNUSED const JPH::Body& inBody2,
    MAYBE_UNUSED const JPH::ContactManifold& inManifold,
    MAYBE_UNUSED JPH::ContactSettings& ioSettings)
{
}

void PhysicsContactListener::OnContactRemoved(MAYBE_UNUSED const JPH::SubShapeIDPair& inSubShapePair)
{
}