#include "physics_bindings.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "physics_module.hpp"
#include "utility/wren_entity.hpp"

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

void AddForce(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody, const glm::vec3& direction, const float amount)
{
    self.AddForce(*rigidBody.component, direction, amount);
}

void AddImpulse(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody, const glm::vec3& direction, const float amount)
{
    self.AddImpulse(*rigidBody.component, direction, amount);
}

void SetVelocity(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody, const glm::vec3& velocity)
{
    self.SetVelocity(*rigidBody.component, velocity);
}

void SetAngularVelocity(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody, const glm::vec3& velocity)
{
    self.SetAngularVelocity(*rigidBody.component, velocity);
}

void SetGravityFactor(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody, const float factor)
{
    self.SetGravityFactor(*rigidBody.component, factor);
}

void SetFriction(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody, const float friction)
{
    self.SetFriction(*rigidBody.component, friction);
}

glm::vec3 GetVelocity(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody)
{
    return self.GetVelocity(*rigidBody.component);
}

glm::vec3 GetAngularVelocity(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody)
{
    return self.GetAngularVelocity(*rigidBody.component);
}

glm::vec3 GetPosition(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody)
{
    return self.GetPosition(*rigidBody.component);
}

glm::vec3 GetRotation(PhysicsModule& self, WrenComponent<RigidbodyComponent>& rigidBody)
{
    return self.GetRotation(*rigidBody.component);
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
    rayHitInfo.propReadonlyExt<bindings::GetHitEntity>("entity");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitPosition>("position");
    rayHitInfo.propReadonlyExt<bindings::GetRayHitNormal>("normal");

    module.klass<JPH::BodyID>("BodyID");

    auto& rigidBodyComponent = module.klass<WrenComponent<RigidbodyComponent>>("RigidbodyComponent");

    rigidBodyComponent.propReadonlyExt<bindings::GetBodyID>("GetBodyID");
}
