#pragma once

#include <entt/entity/registry.hpp>
#include <physics/jolt_to_glm.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

struct UpdateMeshAndPhysics
{
};

class RigidbodyComponent
{
public:
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::ShapeRefC shape, bool dynamic, JPH::EAllowedDOFs freedom = JPH::EAllowedDOFs::All);

    static void SetupRegistryCallbacks(entt::registry& registry);
    static void DisconnectRegistryCallbacks(entt::registry& registry);

    // Getters
    glm::vec3 GetPosition() const { return ToGLMVec3(bodyInterface->GetPosition(bodyID)); }
    glm::quat GetRotation() const { return ToGLMQuat(bodyInterface->GetRotation(bodyID)); }
    glm::vec3 GetVelocity() const { return ToGLMVec3(bodyInterface->GetLinearVelocity(bodyID)); }
    glm::vec3 GetAngularVelocity() const { return ToGLMVec3(bodyInterface->GetLinearVelocity(bodyID)); };

    // Setters
    void SetVelocity(const glm::vec3& velocity) { bodyInterface->SetLinearVelocity(bodyID, ToJoltVec3(velocity)); };
    void SetAngularVelocity(const glm::vec3& velocity) { bodyInterface->SetAngularVelocity(bodyID, ToJoltVec3(velocity)); };
    void SetGravityFactor(float factor) { bodyInterface->SetGravityFactor(bodyID, factor); }
    void SetFriction(float friction) { bodyInterface->SetFriction(bodyID, friction); }
    void SetTranslation(const glm::vec3& translation) { bodyInterface->SetPosition(bodyID, ToJoltVec3(translation), JPH::EActivation::Activate); }
    void SetRotation(const glm::quat& rotation) { bodyInterface->SetRotation(bodyID, ToJoltQuat(rotation), JPH::EActivation::Activate); }

    // Adders
    void AddForce(const glm::vec3& force) { bodyInterface->AddForce(bodyID, ToJoltVec3(force)); }
    void AddImpulse(const glm::vec3& force) { bodyInterface->AddImpulse(bodyID, ToJoltVec3(force)); }

    void SetColliderShape(JPH::ShapeRefC newShape);

    JPH::BodyID bodyID;
    JPH::ShapeRefC shape;

private:
    bool dynamic = false;
    JPH::EAllowedDOFs dofs {};
    JPH::BodyInterface* bodyInterface;

    static void OnDestroyCallback(entt::registry& registry, entt::entity entity);
    static void OnConstructCallback(entt::registry& registry, entt::entity entity);
};