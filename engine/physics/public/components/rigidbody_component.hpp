#pragma once

#include "physics/collision_callback.hpp"
#include "physics/jolt_to_glm.hpp"
#include <entt/entity/registry.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

class RigidbodyComponent
{
public:
    RigidbodyComponent(JPH::BodyInterface& bodyInterface, JPH::ShapeRefC shape, JPH::ObjectLayer layer, JPH::EAllowedDOFs freedom = JPH::EAllowedDOFs::All);

    static void SetupRegistryCallbacks(entt::registry& registry);
    static void DisconnectRegistryCallbacks(entt::registry& registry);

    // Getters
    glm::vec3 GetPosition() const { return ToGLMVec3(bodyInterface->GetPosition(bodyID)); }
    glm::quat GetRotation() const { return ToGLMQuat(bodyInterface->GetRotation(bodyID)); }
    glm::vec3 GetVelocity() const { return ToGLMVec3(bodyInterface->GetLinearVelocity(bodyID)); }
    glm::vec3 GetAngularVelocity() const { return ToGLMVec3(bodyInterface->GetLinearVelocity(bodyID)); };
    JPH::ObjectLayer GetLayer() const { return layer; }

    // Setters
    void SetVelocity(const glm::vec3& velocity) { bodyInterface->SetLinearVelocity(bodyID, ToJoltVec3(velocity)); };
    void SetAngularVelocity(const glm::vec3& velocity) { bodyInterface->SetAngularVelocity(bodyID, ToJoltVec3(velocity)); };
    void SetGravityFactor(float factor) { bodyInterface->SetGravityFactor(bodyID, factor); }
    void SetFriction(float friction) { bodyInterface->SetFriction(bodyID, friction); }
    void SetTranslation(const glm::vec3& translation) { bodyInterface->SetPosition(bodyID, ToJoltVec3(translation), JPH::EActivation::Activate); }
    void SetRotation(const glm::quat& rotation) { bodyInterface->SetRotation(bodyID, ToJoltQuat(glm::normalize(rotation)), JPH::EActivation::Activate); }
    void SetDynamic() { bodyInterface->SetMotionType(bodyID, JPH::EMotionType::Dynamic, JPH::EActivation::Activate); }
    void Setkinematic() { bodyInterface->SetMotionType(bodyID, JPH::EMotionType::Kinematic, JPH::EActivation::Activate); }
    void SetStatic() { bodyInterface->SetMotionType(bodyID, JPH::EMotionType::Static, JPH::EActivation::Activate); }

    // Adders
    void AddForce(const glm::vec3& force) { bodyInterface->AddForce(bodyID, ToJoltVec3(force)); }
    void AddImpulse(const glm::vec3& force) { bodyInterface->AddImpulse(bodyID, ToJoltVec3(force)); }

    void SetColliderShape(JPH::ShapeRefC newShape);

    JPH::BodyID bodyID;
    JPH::ShapeRefC shape;

    CollisionCallback onCollisionEnter;
    CollisionCallback onCollisionStay;

private:
    JPH::ObjectLayer layer {};
    JPH::EAllowedDOFs dofs {};
    JPH::BodyInterface* bodyInterface;

    static void OnDestroyCallback(entt::registry& registry, entt::entity entity);
    static void OnConstructCallback(entt::registry& registry, entt::entity entity);
};