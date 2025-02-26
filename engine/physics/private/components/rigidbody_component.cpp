#include "components/rigidbody_component.hpp"

void RigidbodyComponent::OnConstructCallback(entt::registry& registry, entt::entity entity)
{
    auto& rb = registry.get<RigidbodyComponent>(entity);

    JPH::EMotionType motionType = rb.dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
    JPH::ObjectLayer layer = rb.dynamic ? eMOVING_OBJECT : eNON_MOVING_OBJECT;

    auto scaledShape = rb.shape->ScaleShape(ToJoltVec3(TransformHelpers::GetWorldScale(registry, entity)));

    JPH::BodyCreationSettings creation { scaledShape.Get(),
        ToJoltVec3(TransformHelpers::GetWorldPosition(registry, entity)),
        ToJoltQuat(TransformHelpers::GetWorldRotation(registry, entity)),
        motionType, layer };

    // Needed if we change from a static object to dynamic
    creation.mAllowDynamicOrKinematic = true;
    creation.mAllowedDOFs = rb.dofs;

    // Look into mass settings
    if (rb.shape->GetMassProperties().mMass <= 0.0f)
    {
        creation.mMassPropertiesOverride = JPH::MassProperties(1.0f);
        creation.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    }

    JPH::EActivation activation = rb.dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;

    rb.bodyID = rb.bodyInterface.CreateAndAddBody(creation, activation);
    rb.bodyInterface.SetUserData(rb.bodyID, static_cast<uint64_t>(entity));
}