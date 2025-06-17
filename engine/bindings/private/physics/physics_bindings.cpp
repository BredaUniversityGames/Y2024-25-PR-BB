#include "physics_bindings.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "physics/collision.hpp"
#include "physics/collision_callback.hpp"
#include "physics/physics_bindings.hpp"
#include "physics/shape_factory.hpp"
#include "physics_module.hpp"
#include "profile_macros.hpp"
#include "utility/enum_bind.hpp"
#include "utility/random_util.hpp"
#include "wren_entity.hpp"

#include <glm/gtx/rotate_vector.hpp>

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

JPH::ObjectLayer GetLayer(WrenComponent<RigidbodyComponent>& self)
{
    return self.component->GetLayer();
}

void SetLayer(WrenComponent<RigidbodyComponent>& self, int layer)
{
    self.component->SetLayer(layer);
}

RigidbodyComponent RigidbodyNew(PhysicsModule& physics, JPH::ShapeRefC shape, JPH::ObjectLayer layer, bool allowRotation)
{
    JPH::EAllowedDOFs dofs = allowRotation ? JPH::EAllowedDOFs::All : JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    return RigidbodyComponent { physics.GetBodyInterface(), shape, layer, dofs };
}

void SetOnCollisionEnter(WrenComponent<RigidbodyComponent>& self, wren::Variable callback)
{
    self.component->onCollisionEnter = { callback };
}

void SetOnCollisionStay(WrenComponent<RigidbodyComponent>& self, wren::Variable callback)
{
    self.component->onCollisionStay = { callback };
}

std::optional<glm::vec3> LocalEnemySteering(
    PhysicsModule& physics,
    const WrenComponent<RigidbodyComponent>& self,
    const glm::vec3& forward,
    const glm::vec3& kneeOffset,
    float rayAngle, int rayCount)
{
    auto position = self.component->GetPosition();
    glm::vec3 offsetDirection = glm::vec3(0.0f, 0.0f, 0.0f);

    //--------------------------------------------------------------//
    // Local steering behaviour                                     //
    // Shoots out rays in angle and steers away from closest hits   //
    // 1) Regular behaviour                                         //
    // 2) If all hits are roughly equal steer a hard left or right  //
    // 3) No obstacles in the way, resume regular path              //
    //--------------------------------------------------------------//

    // Find closest ray hit and add this to the offsetDirection vector
    for (int i = 0; i < rayCount; i++)
    {
        auto angleOffset = (i - (rayCount - 1) / 2.0) * glm::radians(rayAngle) / (rayCount / 2);

        auto hitInfos = physics.ShootRay(
            position + kneeOffset,
            glm::rotateY<float>(forward, angleOffset),
            3.0);

        float lowestHitFraction = 1.0;
        float lowestHitFractionIndex = 0;

        for (size_t j = 0; j < hitInfos.size(); j++)
        {
            auto& ray = hitInfos[j];

            if (ray.bodyID == self.component->bodyID)
            {
                continue;
            }

            if (ray.hitFraction < lowestHitFraction)
            {
                lowestHitFraction = ray.hitFraction;
                lowestHitFractionIndex = j;
            }
        }

        if (!hitInfos.empty())
        {
            offsetDirection = offsetDirection + (position - hitInfos[lowestHitFractionIndex].position);
        }
    }

    // If we hit nothing
    if (glm::length(offsetDirection) < 0.001)
    {
        return std::nullopt;
    }

    glm::vec3 out = forward;
    offsetDirection = glm::normalize(offsetDirection);
    auto absDot = glm::abs(glm::dot(forward, offsetDirection));

    if (absDot > 0.9)
    {
        // if the raycast results is a vector that is nearly a negated forward vector
        // steer either hard left or right (random chance for either)

        float angle = 0.0;
        uint32_t index = RandomUtil::RandomIndex(0, 2);
        assert(index == 1 || index == 0);

        if (index == 0)
        {
            angle = glm::radians(-90.0);
        }
        else
        {
            angle = glm::radians(90.0);
        }

        out = glm::rotateY(forward, angle);
    }
    else
    {
        if (absDot < 0.001)
        {
            // if no hits (or too subtle) keep same forward vector
        }
        else
        {
            // regular steering behavior
            out -= offsetDirection;
        }
    }

    return out;
}
}
void BindPhysicsAPI(wren::ForeignModule& module)
{
    // Physics module
    auto& wren_class = module.klass<PhysicsModule>("Physics");

    wren_class.funcExt<bindings::ShootRay>("ShootRay");
    wren_class.funcExt<bindings::ShootMultipleRays>("ShootMultipleRays");
    wren_class.funcExt<bindings::LocalEnemySteering>("LocalEnemySteering",
        "Get a steering direction for an enemy based on raycasts in front of it. Returns nullopt if no raycast hit was found");

    // RayHit struct
    auto& rayHitInfo = module.klass<RayHitInfo>("RayHitInfo");
    rayHitInfo.funcExt<bindings::GetHitEntity>("GetEntity");
    rayHitInfo.var<&RayHitInfo::position>("position");
    rayHitInfo.var<&RayHitInfo::normal>("normal");
    rayHitInfo.var<&RayHitInfo::hitFraction>("hitFraction");

    // Object Layers
    bindings::BindBitflagEnum<PhysicsObjectLayer>(module, "PhysicsObjectLayer");

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
    rigidBodyComponent.funcExt<bindings::GetLayer>("GetLayer");
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
    rigidBodyComponent.funcExt<bindings::SetLayer>("SetLayer");
    rigidBodyComponent.funcExt<bindings::SetOnCollisionEnter>("OnCollisionEnter", "void callback(WrenEntity self, WrenEntity other) -> void");
    rigidBodyComponent.funcExt<bindings::SetOnCollisionStay>("OnCollisionStay", "void callback(WrenEntity self, WrenEntity other) -> void");
}
