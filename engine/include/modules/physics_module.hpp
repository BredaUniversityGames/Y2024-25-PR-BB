// Implementation based on Jolt phusics Hello world example
// https://github.com/jrouwe/JoltPhysicsHelloWorld/blob/main/Source/HelloWorld.cpp

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
#pragma once
#include "Jolt/Jolt.h"
// Jolt includes
#include "../../../build/WSL-Debug/_deps/joltphysics-src/Jolt/Renderer/DebugRendererSimple.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#define JPH_DEBUG_RENDERER
namespace JPH
{
class DebugRendererSimple;
}
JPH_SUPPRESS_WARNINGS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
        case Layers::NON_MOVING:
            return inObject2 == Layers::MOVING; // Non moving only collides with moving
        case Layers::MOVING:
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
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    virtual JPH::uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
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
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
        case Layers::NON_MOVING:
            return inLayer2 == BroadPhaseLayers::MOVING;
        case Layers::MOVING:
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
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override
    {
        std::cout << "Contact validate callback" << std::endl;

        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
    {
        std::cout << "A contact was added" << std::endl;
    }

    virtual void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override
    {
        std::cout << "A contact was persisted" << std::endl;
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
    {
        std::cout << "A contact was removed" << std::endl;
    }
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener
{
public:
    virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
    {
        std::cout << "A body got activated" << std::endl;
    }

    virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
    {
        std::cout << "A body went to sleep" << std::endl;
    }
};

class MyDebugRenderer : public JPH::DebugRendererSimple
{
public:
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
    {

        glm::vec3 fromPos(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
        glm::vec3 toPos(inTo.GetX(), inTo.GetY(), inTo.GetZ());

        glm::vec2 from = WorldToScreen(fromPos, view_projection);
        glm::vec2 to = WorldToScreen(toPos, view_projection);
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        // Convert JPH::ColorArg to ImGui color format
        ImU32 color = IM_COL32(0, 255, 0, 255);

        // Use ImGui to draw the line

        draw_list->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), color);
    }

    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override
    {
        std::cout << "Drawing Text" << std::endl;

        // Implement
    }

    void RenderDebugOverlay()
    {
        // Get display size
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 windowSize = io.DisplaySize;

        // Set window properties
        ImGui::SetNextWindowSize(windowSize); // Fullscreen size
        ImGui::SetNextWindowPos(ImVec2(0, 0)); // Position at the top-left corner

        // Create a window with no title bar, no resize, no move, no scrollbars, and no background
        ImGui::Begin("DebugOverlay", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs | // No mouse/keyboard input
                ImGuiWindowFlags_NoBackground); // No background

        // Draw your debug lines or shapes here

        // Get the foreground draw list
        draw_list = ImGui::GetWindowDrawList();

        // End the window
        ImGui::End();
    }

    void UpdateViewProjection(const glm::mat4& inViewProjection)
    {
        view_projection = inViewProjection;
    }
    ImDrawList* draw_list = nullptr;
    glm::mat4 view_projection;

private:
    glm::vec2 WorldToScreen(const glm::vec3& worldPos, const glm::mat4& viewProjection)
    {
        // Identity model matrix since we're transforming world coordinates directly
        glm::mat4 model = glm::mat4(1.0f);

        // Get ImGui display size
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        // Define the viewport: origin at (0,0), width and height as per the display size
        glm::vec4 viewport = glm::vec4(0.0f, 0.0f, displaySize.x, displaySize.y);

        // Project the 3D point to 2D screen coordinates
        glm::vec3 screenPos = glm::project(worldPos, model, viewProjection, viewport);

        // Adjust for ImGui's coordinate system (origin at top-left)
        // screenPos.y = displaySize.y - screenPos.y;

        return glm::vec2(screenPos.x, screenPos.y);
    }
};

class PhysicsModule
{
public:
    PhysicsModule();
    ~PhysicsModule();
    void UpdatePhysicsEngine(float deltaTime);
    JPH::BodyInterface* body_interface = nullptr;
    MyDebugRenderer* debug_renderer = nullptr;
    JPH::PhysicsSystem* physics_system = nullptr;

private:
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint cMaxBodies = 16384;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const JPH::uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
    // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const JPH::uint cMaxBodyPairs = 16384;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
    // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
    const JPH::uint cMaxContactConstraints = 8192;

    // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
    const int cCollisionSteps = 1;

    MyBodyActivationListener* body_activation_listener = nullptr;
    MyContactListener* contact_listener = nullptr;
    BPLayerInterfaceImpl* broad_phase_layer_interface = nullptr;
    ObjectVsBroadPhaseLayerFilterImpl* object_vs_broadphase_layer_filter = nullptr;
    ObjectLayerPairFilterImpl* object_vs_object_layer_filter = nullptr;
    // for updates
    JPH::TempAllocatorImpl* temp_allocator = nullptr;
    JPH::JobSystemThreadPool* job_system = nullptr;

    // jolt isn't that happy with smart pointers sadly
};