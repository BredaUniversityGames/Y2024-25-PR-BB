// Implementation based on Jolt physics Hello world example
// https://github.com/jrouwe/JoltPhysicsHelloWorld/blob/main/Source/HelloWorld.cpp

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
#pragma once
#include "Jolt/Jolt.h"
#include "common.hpp"
#include "entt/entity/entity.hpp"
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/RayCast.h>

#include "module_interface.hpp"
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>



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
    eCUSTOM,
};

enum BodyType
{
    eDYNAMIC,
    eSTATIC,
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
        if (!_isEnabled)
        {
            return;
        }
        glm::vec3 fromPos(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        glm::vec3 toPos(inTo.GetX(), inTo.GetY(), inTo.GetZ());

        linePositions.push_back(fromPos);
        linePositions.push_back(toPos);
    }

    void AddPersistentLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor)
    {
        if (!_isEnabled)
        {
            return;
        }

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

    NO_DISCARD const std::vector<glm::vec3>& GetLinesData() const
    {
        return linePositions;
    }

    NO_DISCARD const std::vector<glm::vec3>& GetPersistentLinesData() const
    {
        return persistentLinePositions;
    }

    void ClearLines()
    {
        linePositions.clear();
    }

    void SetState(const bool newState) { _isEnabled = newState; }
    bool GetState() const { return _isEnabled; }

private:
    std::vector<glm::vec3> linePositions;
    std::vector<glm::vec3> persistentLinePositions;
    bool _isEnabled = true;
};

struct RayHitInfo
{
    entt::entity entity = entt::null; // Entity that was hit
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); // Position where the ray hits; HitPoint = Start + mFraction * (End - Start)
    float hitFraction = 0.0f; // Hit fraction of the ray/object [0, 1], HitPoint = Start + mFraction * (End - Start)
    bool hasHit = false;
};
inline glm::mat4 ToGLMMat4(const JPH::RMat44& mat)
{
    glm::mat4 glmMat;

    // JPH::RMat44 stores rotation columns and translation separately
    // mRotation is a 3x3 matrix, and mTranslation is a Vec3
    // GLM uses column-major order, so we can map the columns directly

    // Extract rotation columns from JPH::RMat44
    JPH::Vec3 col0 = mat.GetColumn3(0);
    JPH::Vec3 col1 = mat.GetColumn3(1);
    JPH::Vec3 col2 = mat.GetColumn3(2);
    JPH::Vec3 translation = mat.GetTranslation();

    // Set the columns of glm::mat4
    glmMat[0] = glm::vec4(col0.GetX(), col0.GetY(), col0.GetZ(), 0.0f);
    glmMat[1] = glm::vec4(col1.GetX(), col1.GetY(), col1.GetZ(), 0.0f);
    glmMat[2] = glm::vec4(col2.GetX(), col2.GetY(), col2.GetZ(), 0.0f);
    glmMat[3] = glm::vec4(translation.GetX(), translation.GetY(), translation.GetZ(), 1.0f);

    return glmMat;
}
class PhysicsModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;

public:
    PhysicsModule()
        = default;
    ~PhysicsModule() final = default;

    NO_DISCARD RayHitInfo ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const;

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