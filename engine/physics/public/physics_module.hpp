#pragma once

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "common.hpp"
#include "components/rigidbody_component.hpp"
#include "module_interface.hpp"
#include "physics/collision.hpp"
#include "physics/constants.hpp"
#include "physics/contact_listener.hpp"
#include "physics/debug_renderer.hpp"
#include "physics/jolt_to_glm.hpp"
#include "physics/shape_factory.hpp"

#include <entt/entity/entity.hpp>

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
    PhysicsModule() = default;
    ~PhysicsModule() final = default;

    NO_DISCARD std::vector<RayHitInfo> ShootRay(const glm::vec3& origin, const glm::vec3& direction, float distance) const;
    NO_DISCARD std::vector<RayHitInfo> ShootMultipleRays(const glm::vec3& origin, const glm::vec3& direction, float distance, unsigned int numRays, float angle) const;

    JPH::BodyInterface& GetBodyInterface() { return _physicsSystem->GetBodyInterface(); }
    const JPH::BodyInterface& GetBodyInterface() const { return _physicsSystem->GetBodyInterface(); }

    glm::vec3 GetPosition(RigidbodyComponent& rigidBody) const;
    glm::vec3 GetRotation(RigidbodyComponent& rigidBody) const;
    void AddForce(RigidbodyComponent& rigidBody, const glm::vec3& direction, const float amount);
    void AddImpulse(RigidbodyComponent& rigidBody, const glm::vec3& direction, const float amount);
    glm::vec3 GetVelocity(RigidbodyComponent& rigidBody) const;
    glm::vec3 GetAngularVelocity(RigidbodyComponent& rigidBody) const;
    void SetVelocity(RigidbodyComponent& rigidBody, const glm::vec3& velocity);
    void SetAngularVelocity(RigidbodyComponent& rigidBody, const glm::vec3& velocity);
    void SetGravityFactor(RigidbodyComponent& rigidBody, const float factor);
    void SetFriction(RigidbodyComponent& rigidBody, const float friction);

    std::unique_ptr<JPH::PhysicsSystem> _physicsSystem {};
    std::unique_ptr<PhysicsDebugRenderer> _debugRenderer {};

private:
    std::unique_ptr<JPH::ContactListener> _contactListener {};

    std::unique_ptr<JPH::ObjectLayerPairFilter> _objectVsObjectLayerFilter {};
    std::unique_ptr<JPH::BroadPhaseLayerInterface> _broadphaseLayerInterface {};
    std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> _objectVsBroadphaseLayerFilter {};

    std::unique_ptr<JPH::TempAllocator> _tempAllocator;
    std::unique_ptr<JPH::JobSystem> _jobSystem;
};