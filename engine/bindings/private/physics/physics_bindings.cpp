#include "physics_bindings.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "entity/wren_entity.hpp"
#include "physics_module.hpp"

#include <optional>

namespace bindings
{
std::vector<RayHitInfo> ShootRay(PhysicsModule& self, const glm::vec3& origin, const glm::vec3& direction, const float distance)
{
    return self.ShootRay(origin, direction, distance);
}

std::vector<RayHitInfo> ShootMultipleRays(PhysicsModule& self, const glm::vec3& origin, const glm::vec3& direction, const float distance, const unsigned int numRays, const float angle)
{
    return self.ShootMultipleRays(origin, direction, distance, numRays, angle);
}

void AddForce(WrenComponent<RigidbodyComponent>& self, const glm::vec3& force)
{
    self.component->AddForce(force);
}

void AddImpulse(WrenComponent<RigidbodyComponent>& self, const glm::vec3& force)
{
    self.component->AddImpulse(force);
}

void SetVelocity(WrenComponent<RigidbodyComponent>& self, const glm::vec3& velocity)
{
    self.component->SetVelocity(velocity);
}

void SetAngularVelocity(WrenComponent<RigidbodyComponent>& self, const glm::vec3& velocity)
{
    self.component->SetAngularVelocity(velocity);
}

void SetGravityFactor(WrenComponent<RigidbodyComponent>& self, float factor)
{
    self.component->SetGravityFactor(factor);
}

void SetFriction(WrenComponent<RigidbodyComponent>& self, const float friction)
{
    self.component->SetFriction(friction);
}

glm::vec3 GetVelocity(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->GetVelocity();
}

glm::vec3 GetAngularVelocity(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->GetAngularVelocity();
}

glm::vec3 GetPosition(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->GetPosition();
}

glm::quat GetRotation(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->GetRotation();
}

WrenEntity GetHitEntity(RayHitInfo& self, ECSModule& ecs)
{
    return WrenEntity { self.entity, &ecs.GetRegistry() };
}

glm::vec3 GetRayHitPosition(RayHitInfo& self)
{
    return self.position;
}

glm::vec3 GetRayHitNormal(RayHitInfo& self)
{
    return self.normal;
}

JPH::BodyID GetBodyID(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->bodyID;
}
}
void BindPhysicsAPI(wren::ForeignModule& module)
{
    // Physics module
    auto& wren_class = module.klass<PhysicsModule>("Physics");

    wren_class.funcExt<bindings::ShootRay>("ShootRay");
    wren_class.funcExt<bindings::ShootMultipleRays>("ShootMultipleRays");

    // RayHit struct
    auto& rayHitInfo = module.klass<RayHitInfo>("RayHitInfo");
    rayHitInfo.funcExt<bindings::GetHitEntity>("GetEntity");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitPosition>("position");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitNormal>("normal");

    // Body ID (why is this needed?)
    module.klass<JPH::BodyID>("BodyID");

    // Rigidbody component
    auto& rigidBodyComponent = module.klass<WrenComponent<RigidbodyComponent>>("RigidbodyComponent");
    rigidBodyComponent.propReadonlyExt<bindings::GetBodyID>("GetBodyID");

    rigidBodyComponent.funcExt<bindings::AddForce>("AddForce");
    rigidBodyComponent.funcExt<bindings::AddImpulse>("AddImpulse");
    rigidBodyComponent.funcExt<bindings::GetVelocity>("GetVelocity");
    rigidBodyComponent.funcExt<bindings::GetAngularVelocity>("GetAngularVelocity");
    rigidBodyComponent.funcExt<bindings::SetVelocity>("SetVelocity");
    rigidBodyComponent.funcExt<bindings::SetAngularVelocity>("SetAngularVelocity");
    rigidBodyComponent.funcExt<bindings::GetPosition>("GetPosition");
    rigidBodyComponent.funcExt<bindings::GetRotation>("GetRotation");
    rigidBodyComponent.funcExt<bindings::SetGravityFactor>("SetGravityFactor");
    rigidBodyComponent.funcExt<bindings::SetFriction>("SetFriction");
}
