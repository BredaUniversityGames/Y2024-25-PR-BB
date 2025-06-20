﻿#include "components/rigidbody_component.hpp"

#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "physics/collision.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

RigidbodyComponent::RigidbodyComponent(JPH::BodyInterface& bodyInterface,
    JPH::ShapeRefC shape,
    JPH::ObjectLayer layer,
    JPH::EAllowedDOFs freedom)
    : shape(shape)
    , layer(layer)
    , dofs(freedom)
    , bodyInterface(&bodyInterface)
{
}

void RigidbodyComponent::OnConstructCallback(entt::registry& registry, entt::entity entity)
{
    auto& rb = registry.get<RigidbodyComponent>(entity);

    JPH::EMotionType motionType = rb.layer == eSTATIC ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;
    auto scaledShape = rb.shape->ScaleShape(ToJoltVec3(TransformHelpers::GetWorldScale(registry, entity)));

    JPH::BodyCreationSettings creation { scaledShape.Get(),
        ToJoltVec3(TransformHelpers::GetWorldPosition(registry, entity)),
        ToJoltQuat(TransformHelpers::GetWorldRotation(registry, entity)),
        motionType, rb.layer };

    // Needed if we change from a static object to dynamic
    creation.mAllowDynamicOrKinematic = true;
    creation.mAllowedDOFs = rb.dofs;

    // Look into mass settings
    if (rb.shape->GetMassProperties().mMass <= 0.0f)
    {
        creation.mMassPropertiesOverride = JPH::MassProperties { 1.0f, JPH::Mat44::sZero() };
        creation.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    }

    JPH::EActivation activation = motionType == JPH::EMotionType::Dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;

    rb.bodyID = rb.bodyInterface->CreateAndAddBody(creation, activation);
    rb.bodyInterface->SetUserData(rb.bodyID, static_cast<uint64_t>(entity));
}

void RigidbodyComponent::OnDestroyCallback(entt::registry& registry, entt::entity entity)
{
    auto& rb = registry.get<RigidbodyComponent>(entity);

    if (!rb.bodyID.IsInvalid())
    {
        rb.bodyInterface->RemoveBody(rb.bodyID);
        rb.bodyInterface->DestroyBody(rb.bodyID);
    }
}

void RigidbodyComponent::SetupRegistryCallbacks(entt::registry& registry)
{
    registry.on_construct<RigidbodyComponent>().connect<OnConstructCallback>();
    registry.on_destroy<RigidbodyComponent>().connect<OnDestroyCallback>();
}

void RigidbodyComponent::DisconnectRegistryCallbacks(entt::registry& registry)
{
    registry.on_construct<RigidbodyComponent>().disconnect<OnConstructCallback>();
    registry.on_destroy<RigidbodyComponent>().disconnect<OnDestroyCallback>();
}