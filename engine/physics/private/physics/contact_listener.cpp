#include "physics/contact_listener.hpp"
#include "Jolt/Physics/Body/Body.h"
#include "components/rigidbody_component.hpp"

JPH::ValidateResult PhysicsContactListener::OnContactValidate(
    MAYBE_UNUSED const JPH::Body& inBody1,
    MAYBE_UNUSED const JPH::Body& inBody2,
    MAYBE_UNUSED JPH::RVec3Arg inBaseOffset,
    MAYBE_UNUSED const JPH::CollideShapeResult& inCollisionResult)
{
    // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
    return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
}

void PhysicsContactListener::OnContactAdded(
    const JPH::Body& inBody1,
    const JPH::Body& inBody2,
    MAYBE_UNUSED const JPH::ContactManifold& inManifold,
    MAYBE_UNUSED JPH::ContactSettings& ioSettings)
{
    std::scoped_lock<std::mutex> lock { callbackMutex };

    auto e1 = static_cast<entt::entity>(inBody1.GetUserData());
    auto e2 = static_cast<entt::entity>(inBody2.GetUserData());

    auto rb1 = registry.get<RigidbodyComponent>(e1);
    auto rb2 = registry.get<RigidbodyComponent>(e2);

    rb1.onCollisionEnter(WrenEntity { e1, &registry }, WrenEntity { e2, &registry });
    rb2.onCollisionEnter(WrenEntity { e2, &registry }, WrenEntity { e1, &registry });
}

void PhysicsContactListener::OnContactPersisted(
    const JPH::Body& inBody1,
    const JPH::Body& inBody2,
    MAYBE_UNUSED const JPH::ContactManifold& inManifold,
    MAYBE_UNUSED JPH::ContactSettings& ioSettings)
{
    std::scoped_lock<std::mutex> lock { callbackMutex };

    auto e1 = static_cast<entt::entity>(inBody1.GetUserData());
    auto e2 = static_cast<entt::entity>(inBody2.GetUserData());

    auto rb1 = registry.get<RigidbodyComponent>(e1);
    auto rb2 = registry.get<RigidbodyComponent>(e2);

    rb1.onCollisionStay(WrenEntity { e1, &registry }, WrenEntity { e2, &registry });
    rb2.onCollisionStay(WrenEntity { e2, &registry }, WrenEntity { e1, &registry });
}

void PhysicsContactListener::OnContactRemoved(MAYBE_UNUSED const JPH::SubShapeIDPair& inSubShapePair)
{
    // Unimplemented
}