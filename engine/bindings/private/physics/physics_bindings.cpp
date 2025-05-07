#include "physics_bindings.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "entity/wren_entity.hpp"
#include "physics/collision.hpp"
#include "physics/physics_bindings.hpp"
#include "physics/shape_factory.hpp"
#include "physics_module.hpp"
#include "utility/enum_bind.hpp"

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

void SetTranslation(WrenComponent<RigidbodyComponent>& self, const glm::vec3& translation)
{
    self.component->SetTranslation(translation);
}

void SetRotation(WrenComponent<RigidbodyComponent>& self, const glm::quat rotation)
{
    self.component->SetRotation(rotation);
}

void SetDynamic(WrenComponent<RigidbodyComponent>& self)
{
    self.component->SetDynamic();
}

void SetKinematic(WrenComponent<RigidbodyComponent>& self)
{
    self.component->Setkinematic();
}

void SetStatic(WrenComponent<RigidbodyComponent>& self)
{
    self.component->SetStatic();
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

JPH::BodyID GetBodyID(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->bodyID;
}

RigidbodyComponent RigidbodyNew(PhysicsModule& physics, JPH::ShapeRefC shape, PhysicsObjectLayer layer, bool allowRotation)
{
    JPH::EAllowedDOFs dofs = allowRotation ? JPH::EAllowedDOFs::All : JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    return RigidbodyComponent { physics.GetBodyInterface(), shape, layer, dofs };
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
    rayHitInfo.var<&RayHitInfo::position>("position");
    rayHitInfo.var<&RayHitInfo::normal>("normal");
    rayHitInfo.var<&RayHitInfo::hitFraction>("hitFraction");

    // Object Layers
    bindings::BindEnum<PhysicsObjectLayer>(module, "PhysicsObjectLayer");

    // Body ID
    module.klass<JPH::BodyID>("BodyID");

    // Shape Ref
    module.klass<JPH::ShapeRefC>("CollisionShape");

    // Shape factory
    auto& shapeFactory = module.klass<ShapeFactory>("ShapeFactory");
    shapeFactory.funcStaticExt<ShapeFactory::MakeBoxShape>("MakeBoxShape");
    shapeFactory.funcStaticExt<ShapeFactory::MakeCapsuleShape>("MakeCapsuleShape");
    shapeFactory.funcStaticExt<ShapeFactory::MakeSphereShape>("MakeSphereShape");
    shapeFactory.funcStaticExt<ShapeFactory::MakeConvexHullShape>("MakeConvexHullShape");
    shapeFactory.funcStaticExt<ShapeFactory::MakeMeshHullShape>("MakeMeshHullShape");

    // Rigidbody component (a bit hacky, since we cannot add a default constructed rb to the ECS)

    auto& rigidBody = module.klass<RigidbodyComponent>("Rigidbody");
    rigidBody.funcStaticExt<bindings::RigidbodyNew>("new", "Construct a Rigidbody by providing the Physics System, a collision shape, whether it is dynamic and if we want to allow rotation");

    auto& rigidBodyComponent = module.klass<WrenComponent<RigidbodyComponent>>("RigidbodyComponent", "Must be created by passing a Rigidbody to the AddComponent function on an entity");
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
    rigidBodyComponent.funcExt<bindings::SetTranslation>("SetTranslation");
    rigidBodyComponent.funcExt<bindings::SetRotation>("SetRotation");
    rigidBodyComponent.funcExt<bindings::SetDynamic>("SetDynamic");
    rigidBodyComponent.funcExt<bindings::SetKinematic>("SetDynamic");
    rigidBodyComponent.funcExt<bindings::SetStatic>("SetStatic");
}
