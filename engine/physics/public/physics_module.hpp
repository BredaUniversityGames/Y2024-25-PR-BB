// Implementation based on Jolt phusics Hello world example
// https://github.com/jrouwe/JoltPhysicsHelloWorld/blob/main/Source/HelloWorld.cpp

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
#pragma once

#include "Jolt/Jolt.h"
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>

JPH_SUPPRESS_WARNING_PUSH

JPH_SUPPRESS_WARNINGS

#include "Jolt/Renderer/DebugRendererSimple.h"
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

JPH_SUPPRESS_WARNING_POP

// TODO: should be using Log.hpp
#include "common.hpp"
#include <glm/glm.hpp>
#include <iostream>

namespace JPH
{
class DebugRendererSimple;
}
// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace PhysicsLayers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

enum PhysicsShapes
{
    eSPHERE,
    eBOX,
    eCONVEXHULL,
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case PhysicsLayers::NON_MOVING:
            return inObject2 == PhysicsLayers::MOVING; // Non moving only collides with moving
        case PhysicsLayers::MOVING:
            return true; // Moving collides with everything
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr JPH::uint NUM_LAYERS(2);
};
// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[PhysicsLayers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[PhysicsLayers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < PhysicsLayers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
            return "NON_MOVING";
        case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
            return "MOVING";
        default:
            JPH_ASSERT(false);
            return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[PhysicsLayers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case PhysicsLayers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case PhysicsLayers::MOVING:
            return true;
        default:
            JPH_ASSERT(false);
            return false;
        }
    }
};

// An example contact listener
class MyContactListener : public JPH::ContactListener
{
public:
    // See: ContactListener
    virtual JPH::ValidateResult OnContactValidate(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED JPH::RVec3Arg inBaseOffset,
        MAYBE_UNUSED const JPH::CollideShapeResult& inCollisionResult) override
    {
        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED const JPH::ContactManifold& inManifold,
        MAYBE_UNUSED JPH::ContactSettings& ioSettings) override
    {
    }

    virtual void OnContactPersisted(
        MAYBE_UNUSED const JPH::Body& inBody1,
        MAYBE_UNUSED const JPH::Body& inBody2,
        MAYBE_UNUSED const JPH::ContactManifold& inManifold,
        MAYBE_UNUSED JPH::ContactSettings& ioSettings) override
    {
    }

    virtual void OnContactRemoved(MAYBE_UNUSED const JPH::SubShapeIDPair& inSubShapePair) override
    {
    }
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
    virtual void OnBodyActivated(MAYBE_UNUSED const JPH::BodyID& inBodyID, MAYBE_UNUSED JPH::uint64 inBodyUserData) override
    {
    }

    virtual void OnBodyDeactivated(MAYBE_UNUSED const JPH::BodyID& inBodyID, MAYBE_UNUSED JPH::uint64 inBodyUserData) override
    {
    }
};

class DebugRendererSimpleImpl : public JPH::DebugRendererSimple
{
public:
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor) override
    {
        glm::vec3 fromPos(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        glm::vec3 toPos(inTo.GetX(), inTo.GetY(), inTo.GetZ());

        linePositions.push_back(fromPos);
        linePositions.push_back(toPos);
    }

    void AddPersistentLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor)
    {
        glm::vec3 fromPos(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        glm::vec3 toPos(inTo.GetX(), inTo.GetY(), inTo.GetZ());

        persistentLinePositions.push_back(fromPos);
        persistentLinePositions.push_back(toPos);
    }

    void DrawText3D(
        MAYBE_UNUSED JPH::RVec3Arg inPosition,
        MAYBE_UNUSED const std::string_view& inString,
        MAYBE_UNUSED JPH::ColorArg inColor,
        MAYBE_UNUSED float inHeight) override
    {
        // Not implemented
    }

    [[nodiscard]] const std::vector<glm::vec3>& GetLinesData() const
    {
        return linePositions;
    }

    [[nodiscard]] const std::vector<glm::vec3>& GetPersistentLinesData() const
    {
        return persistentLinePositions;
    }

    void ClearLines()
    {
        linePositions.clear();
    }

private:
    std::vector<glm::vec3> linePositions;
    std::vector<glm::vec3> persistentLinePositions;
};

class PhysicsModule
{
public:
    PhysicsModule();
    ~PhysicsModule();
    void UpdatePhysicsEngine(float deltaTime);
    JPH::BodyInterface* bodyInterface = nullptr;
    DebugRendererSimpleImpl* debugRenderer = nullptr;
    JPH::PhysicsSystem* physicsSystem = nullptr;

private:
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint _maxBodies = 16384;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const JPH::uint _numBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint _maxBodyPairs = 16384;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
    const JPH::uint _maxContactConstraints = 8192;

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int _collisionSteps = 1;

    MyBodyActivationListener* _bodyActivationListener = nullptr;
    MyContactListener* _contactListener = nullptr;
    BPLayerInterfaceImpl* _broadPhaseLayerInterface = nullptr;
    ObjectVsBroadPhaseLayerFilterImpl* _objectVsBroadphaseLayerFilter = nullptr;
    ObjectLayerPairFilterImpl* _objectVsObjectLayerFilter = nullptr;
    // for updates
    JPH::TempAllocatorImpl* _tempAllocator = nullptr;
    JPH::JobSystemThreadPool* _jobSystem = nullptr;

    // jolt isn't that happy with smart pointers sadly
};