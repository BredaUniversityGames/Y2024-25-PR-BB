#pragma once

#include "common.hpp"
#include "module_interface.hpp"

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>
#include <unordered_map>

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

class PhysicsDebugRenderer;

struct RayHitInfo
{
    entt::entity entity = entt::null; // entity that was hit
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); // Position where the ray hits; HitPoint = Start + mFraction * (End - Start)
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f); // Normal of the hit surface
    float hitFraction = 0.0f; // Hit fraction of the ray/object [0, 1], HitPoint = Start + mFraction * (End - Start)
};

class PhysicsModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) final;
    void Shutdown(Engine& engine) final;
    void Tick(Engine& engine) final;
    std::string_view GetName() override { return "Physics Module"; }

public:
    PhysicsModule();
    ~PhysicsModule() final;

    NO_DISCARD std::vector<RayHitInfo> ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const;
    NO_DISCARD std::vector<RayHitInfo> ShootMultipleRays(const glm::vec3& origin, const glm::vec3& direction, float distance, unsigned int numRays, float angle) const;

    JPH::BodyInterface& GetBodyInterface() { return _physicsSystem->GetBodyInterface(); }
    const JPH::BodyInterface& GetBodyInterface() const { return _physicsSystem->GetBodyInterface(); }

    void SetDebugCameraPosition(const glm::vec3& cameraPos) const;
    void ResetPersistentDebugLines();

    std::unordered_set<JPH::ObjectLayer> _debugLayersToRender {};
    bool _drawRays = false;
    bool _clearDrawnRaysPerFrame = false;

    std::unique_ptr<JPH::PhysicsSystem> _physicsSystem {};

private:
    std::unique_ptr<JPH::ContactListener> _contactListener {};
    std::unique_ptr<PhysicsDebugRenderer> _debugRenderer {};

    std::unique_ptr<JPH::ObjectLayerPairFilter> _objectVsObjectLayerFilter {};
    std::unique_ptr<JPH::BroadPhaseLayerInterface> _broadphaseLayerInterface {};
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> _objectVsBroadphaseLayerFilter {};

    std::unique_ptr<JPH::TempAllocator> _tempAllocator;
    std::unique_ptr<JPH::JobSystem> _jobSystem;
};