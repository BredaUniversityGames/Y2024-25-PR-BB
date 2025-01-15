#include "physics_bindings.hpp"
#include "components/rigidbody_component.hpp"
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

void AddForce(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& direction, const float amount)
{
    self.AddForce(rigidBody, direction, amount);
}

void AddImpulse(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& direction, const float amount)
{
    self.AddImpulse(rigidBody, direction, amount);
}

void SetVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& velocity)
{
    self.SetVelocity(rigidBody, velocity);
}

void SetAngularVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& velocity)
{
    self.SetAngularVelocity(rigidBody, velocity);
}

void SetGravityFactor(PhysicsModule& self, RigidbodyComponent& rigidBody, const float factor)
{
    self.SetGravityFactor(rigidBody, factor);
}

void SetFriction(PhysicsModule& self, RigidbodyComponent& rigidBody, const float friction)
{
    self.SetFriction(rigidBody, friction);
}

glm::vec3 GetVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody)
{
    return self.GetVelocity(rigidBody);
}

glm::vec3 GetAngularVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody)
{
    return self.GetAngularVelocity(rigidBody);
}

glm::vec3 GetPosition(PhysicsModule& self, RigidbodyComponent& rigidBody)
{
    return self.GetPosition(rigidBody);
}

glm::vec3 GetRotation(PhysicsModule& self, RigidbodyComponent& rigidBody)
{
    return self.GetRotation(rigidBody);
}

entt::entity GetHitEntity(RayHitInfo& self)
{
    return self.entity;
}

glm::vec3 GetRayHitPosition(RayHitInfo& self)
{
    return self.position;
}

glm::vec3 GetRayHitNormal(RayHitInfo& self)
{
    return self.normal;
}

JPH::BodyID GetBodyID(RigidbodyComponent& self)
{
    return self.bodyID;
}
}
void BindPhysicsAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<PhysicsModule>("Physics");
    wren_class.funcExt<bindings::ShootRay>("ShootRay");
    wren_class.funcExt<bindings::ShootMultipleRays>("ShootMultipleRays");
    wren_class.funcExt<bindings::AddForce>("AddForce");
    wren_class.funcExt<bindings::AddImpulse>("AddImpulse");
    wren_class.funcExt<bindings::GetVelocity>("GetVelocity");
    wren_class.funcExt<bindings::GetAngularVelocity>("GetAngularVelocity");
    wren_class.funcExt<bindings::SetVelocity>("SetVelocity");
    wren_class.funcExt<bindings::SetAngularVelocity>("SetAngularVelocity");
    wren_class.funcExt<bindings::GetPosition>("GetPosition");
    wren_class.funcExt<bindings::GetRotation>("GetRotation");
    wren_class.funcExt<bindings::SetGravityFactor>("GravityFactor");
    wren_class.funcExt<bindings::SetFriction>("SetFriction");

    auto& rayHitInfo = module.klass<RayHitInfo>("RayHitInfo");
    rayHitInfo.propReadonlyExt<bindings::GetHitEntity>("hitEntity");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitPosition>("position");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitNormal>("normal");

    auto& rigidBodyComponent = module.klass<RigidbodyComponent>("RigidbodyComponent");
    rigidBodyComponent.propReadonlyExt<bindings::GetBodyID>("GetBodyID");
}
