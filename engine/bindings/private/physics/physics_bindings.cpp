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

void AddForce(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& direction, const float amount)
{
    self.AddForce(rigidBody, direction, amount);
}

void AddImpulse(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& direction, const float amount)
{
    self.AddImpulse(rigidBody, direction, amount);
}

glm::vec3 GetVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody)
{
    return self.GetVelocity(rigidBody);
}

glm::vec3 GetAngularVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody)
{
    return self.GetAngularVelocity(rigidBody);
}

void SetVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& velocity)
{
    self.SetVelocity(rigidBody, velocity);
}

void SetAngularVelocity(PhysicsModule& self, RigidbodyComponent& rigidBody, const glm::vec3& velocity)
{
    self.SetAngularVelocity(rigidBody, velocity);
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
    wren_class.funcExt<bindings::AddForce>("AddForce");
    wren_class.funcExt<bindings::AddImpulse>("AddImpulse");
    wren_class.funcExt<bindings::GetVelocity>("GetVelocity");
    wren_class.funcExt<bindings::GetAngularVelocity>("GetAngularVelocity");
    wren_class.funcExt<bindings::SetVelocity>("SetVelocity");
    wren_class.funcExt<bindings::SetAngularVelocity>("SetAngularVelocity");

    auto& rayHitInfo = module.klass<RayHitInfo>("RayHitInfo");
    rayHitInfo.propReadonlyExt<bindings::GetHitEntity>("hitEntity");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitPosition>("position");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitNormal>("normal");

    auto& rigidBodyComponent = module.klass<RigidbodyComponent>("RigidbodyComponent");
    rigidBodyComponent.propReadonlyExt<bindings::GetBodyID>("GetBodyID");
}
