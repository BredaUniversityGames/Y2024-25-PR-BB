#pragma once

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>

// TODO: should be using Log.hpp

#include "common.hpp"
#include "entt/entity/entity.hpp"
#include "module_interface.hpp"
#include "physics/collision.hpp"
#include "physics/constants.hpp"
#include "physics/contact_listener.hpp"
#include "physics/debug_renderer.hpp"

#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct RayHitInfo
{
    entt::entity entity = entt::null; // entity that was hit
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f); // Position where the ray hits; HitPoint = Start + mFraction * (End - Start)
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 0.0f); // Normal of the hit surface
    float hitFraction = 0.0f; // Hit fraction of the ray/object [0, 1], HitPoint = Start + mFraction * (End - Start)
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
inline glm::vec3 ToGLMVec3(const JPH::Vec3& vec)
{
    return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

struct RigidbodyComponent;
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