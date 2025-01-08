#include "components/rigidbody_component.hpp"

// default creates a sphere at 0,2,0
RigidbodyComponent::RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, PhysicsShapes shapeType, BodyType type)
    : shapeType(shapeType)
    , bodyType(type)
{
    JPH::BodyCreationSettings bodySettings;

    if (shapeType == eSPHERE)
    {
        if (bodyType == eDYNAMIC)
        {
            bodySettings = JPH::BodyCreationSettings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
        }
        else if (bodyType == eSTATIC)
        {
            bodySettings = JPH::BodyCreationSettings(new JPH::SphereShape(0.5f), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
        }

        bodySettings.mAllowDynamicOrKinematic = true;
        bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
    }
    else if (shapeType == eBOX)
    {
        if (bodyType == eDYNAMIC)
        {
            bodySettings = JPH::BodyCreationSettings(new JPH::BoxShape(JPH::Vec3(0.5, 0.5, 0.5)), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
        }
        else if (bodyType == eSTATIC)
        {
            bodySettings = JPH::BodyCreationSettings(new JPH::BoxShape(JPH::Vec3(0.5, 0.5, 0.5)), JPH::Vec3(0.0, 2.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);
        }

        // lets save thes shape reference
        shape = bodySettings.GetShape();

        bodySettings.mAllowDynamicOrKinematic = true;
        bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
    }

    // set the owner entity so we can query it later from physics objects if needed
    bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
}

// for mesh collisions
RigidbodyComponent::RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, JPH::VertexList& vertices, JPH::IndexedTriangleList& triangles)
    : shapeType(eBOX)
    , bodyType(eSTATIC)
{
    JPH::BodyCreationSettings bodySettings;

    JPH::MeshShapeSettings meshSettings;

    if (bodyType == eSTATIC)
    {
        bodySettings = JPH::BodyCreationSettings(
            new JPH::MeshShapeSettings(vertices, triangles),
            JPH::Vec3Arg(position.x, position.y, position.z),
            JPH::QuatArg::sIdentity(),
            JPH::EMotionType::Static,
            PhysicsLayers::NON_MOVING);
    }

    // lets save thes shape reference
    shape = bodySettings.GetShape();

    bodySettings.mAllowDynamicOrKinematic = false;
    bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    // set the owner entity so we can query it later from physics objects if needed
    bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
}

// for convex collisions
RigidbodyComponent::RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, JPH::VertexList& vertices)
    : shapeType(eBOX)
    , bodyType(eSTATIC)
{
    JPH::BodyCreationSettings bodySettings;

    JPH::MeshShapeSettings meshSettings;

    if (bodyType == eSTATIC)
    {
        JPH::Array<JPH::Vec3> hull;

        for (auto vert : vertices)
        {
            hull.push_back(JPH::Vec3(vert.x, vert.y, vert.z));
        }
        bodySettings = JPH::BodyCreationSettings(
            new JPH::ConvexHullShapeSettings(hull),
            JPH::Vec3Arg(position.x, position.y, position.z),
            JPH::QuatArg::sIdentity(),
            JPH::EMotionType::Static,
            PhysicsLayers::NON_MOVING);
    }

    // lets save thes shape reference
    shape = bodySettings.GetShape();
    bodySettings.mAllowDynamicOrKinematic = true;
    bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    // set the owner entity so we can query it later from physics objects if needed
    bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
}

// for AABB collisions
RigidbodyComponent::RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, glm::vec3 position, math::Vec3Range boundingBox, BodyType type)
    : shapeType(eBOX)
    , bodyType(type)
{
    glm::vec3 halfExtents = (boundingBox.max - boundingBox.min) * 0.5f;
    JPH::BodyCreationSettings bodySettings;
    halfExtents = glm::abs(halfExtents);
    if (bodyType == eSTATIC)
    {
        bodySettings = JPH::BodyCreationSettings(
            new JPH::BoxShape(JPH::Vec3Arg(halfExtents.x, halfExtents.y, halfExtents.z), 0.01f),
            JPH::Vec3Arg(position.x, position.y, position.z),
            JPH::QuatArg::sIdentity(),
            JPH::EMotionType::Static,
            PhysicsLayers::NON_MOVING);
    }
    else if (bodyType == eDYNAMIC)
    {
        bodySettings = JPH::BodyCreationSettings(
            new JPH::BoxShape(JPH::Vec3(halfExtents.x, halfExtents.y, halfExtents.z), 0.01f),
            JPH::Vec3(position.x, position.y, position.z),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            PhysicsLayers::MOVING);
    }

    // lets save thes shape reference
    shape = bodySettings.GetShape();
    bodySettings.mAllowDynamicOrKinematic = true;
    bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

    // set the owner entity so we can query it later from physics objects if needed
    bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
}

RigidbodyComponent::RigidbodyComponent(JPH::BodyInterface& bodyInterface, entt::entity ownerEntity, JPH::BodyCreationSettings& bodyCreationSettings)
    : shapeType(eCUSTOM)
{
    // lets save thes shape reference
    shape = bodyCreationSettings.GetShape();
    bodyID = bodyInterface.CreateAndAddBody(bodyCreationSettings, JPH::EActivation::Activate);

    // set the owner entity so we can querry it later from physics ohbejcts if needed
    bodyInterface.SetUserData(bodyID, static_cast<uintptr_t>(ownerEntity));
}