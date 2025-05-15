
#include "systems/physics_system.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/static_mesh_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/matrix_decompose.hpp"
#include "graphics_context.hpp"
#include "imgui.h"
#include "model_loading.hpp"
#include "physics/collision.hpp"
#include "physics/constants.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "resource_management/mesh_resource_manager.hpp"

#include <systems/physics_system.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <components/skinned_mesh_component.hpp>

#include <tracy/Tracy.hpp>

PhysicsSystem::PhysicsSystem(Engine& engine, ECSModule& ecs, PhysicsModule& physicsModule)
    : engine(engine)
    , _ecs(ecs)
    , _physicsModule(physicsModule)
{
}

void PhysicsSystem::Update(MAYBE_UNUSED ECSModule& ecs, MAYBE_UNUSED float deltaTime)
{
    // let's check priority first between transforms and physics
    // Here we update jolt transforms based on our transform system since they are static objects and we want hierarchy
    const auto transformsView = ecs.GetRegistry().view<TransformComponent, RigidbodyComponent, ToBeUpdated, StaticMeshComponent>();
    for (auto entity : transformsView)
    {
        const RigidbodyComponent& rb = transformsView.get<RigidbodyComponent>(entity);

        glm::mat4 worldMatrix = TransformHelpers::GetWorldMatrix(_ecs.GetRegistry(), entity);
        glm::vec3 scale, translation, skew;
        glm::quat rotationQuat;
        glm::vec4 perspective;

        // Decompose the matrix
        glm::decompose(worldMatrix, scale, rotationQuat, translation, skew, perspective);

        _physicsModule.GetBodyInterface().SetPosition(rb.bodyID, JPH::Vec3(translation.x, translation.y, translation.z), JPH::EActivation::Activate);
        _physicsModule.GetBodyInterface().SetRotation(rb.bodyID, JPH::Quat(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w), JPH::EActivation::Activate);

        auto result = rb.shape->ScaleShape(JPH::Vec3Arg(scale.x, scale.y, scale.z));
        if (result.HasError())
            bblog::error(result.GetError().c_str());
        _physicsModule.GetBodyInterface().SetShape(rb.bodyID, result.Get(), false, JPH::EActivation::Activate);
    }

    // this part should be fast because it returns a vector of just ids not whole rigidbodies
    JPH::BodyIDVector activeBodies;
    _physicsModule._physicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

    // mark all active bodies to have their mesh updated
    for (auto active_body : activeBodies)
    {
        const entt::entity entity = static_cast<entt::entity>(_physicsModule.GetBodyInterface().GetUserData(active_body));
        if (_ecs.GetRegistry().valid(entity) && _physicsModule.GetBodyInterface().GetMotionType(active_body) != JPH::EMotionType::Static)
        {
            _ecs.GetRegistry().emplace_or_replace<UpdateMeshAndPhysics>(entity);
        }
    }

    // We now update our transform system to match jolt's since the loop below us handles the dynamic objects that are being simulated by physics
    const auto view = _ecs.GetRegistry().view<RigidbodyComponent, TransformComponent, UpdateMeshAndPhysics>();
    for (const auto entity : view)
    {

        if (_ecs.GetRegistry().all_of<SkinnedMeshComponent>(entity))
            continue;

        const RigidbodyComponent& rb = view.get<RigidbodyComponent>(entity);

        // if somehow is now not active or is static lets remove the update component
        if (!_physicsModule.GetBodyInterface().IsActive(rb.bodyID))
            _ecs.GetRegistry().remove<UpdateMeshAndPhysics>(entity);
        if (_physicsModule.GetBodyInterface().GetMotionType(rb.bodyID) == JPH::EMotionType::Static)
            _ecs.GetRegistry().remove<UpdateMeshAndPhysics>(entity);

        auto joltMatrix = _physicsModule.GetBodyInterface().GetWorldTransform(rb.bodyID);

        // We cant support objects simulated by jolt and our hierarchy system at the same time
        RelationshipComponent* relationship = _ecs.GetRegistry().try_get<RelationshipComponent>(entity);

        if (relationship && relationship->parent != entt::null)
            RelationshipHelpers::DetachChild(_ecs.GetRegistry(), relationship->parent, entity);

        // Crazy jolt stuff that I dont like but it works to set the proper scale
        JPH::BodyLockWrite lock(_physicsModule._physicsSystem->GetBodyLockInterface(), rb.bodyID);
        if (lock.Succeeded())
        {
            JPH::Body& body = lock.GetBody();
            const JPH::ScaledShape* scaled_shape = static_cast<const JPH::ScaledShape*>(body.GetShape());

            const auto joltScale = scaled_shape->GetScale();
            joltMatrix = joltMatrix.PreScaled(joltScale);
        }

        const auto joltToGlm = ToGLMMat4(joltMatrix);

        TransformHelpers::SetWorldTransform(ecs.GetRegistry(), entity, joltToGlm);
    }

    // Clear the tags, if needed the system will add them back again
    TransformHelpers::ResetAllUpdateTags(ecs.GetRegistry());
}
void PhysicsSystem::Render(MAYBE_UNUSED const ECSModule& ecs) const
{
}
void PhysicsSystem::Inspect()
{
    ZoneScoped;
    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Physics System", nullptr, ImGuiWindowFlags_NoResize);
    const auto view = _ecs.GetRegistry().view<RigidbodyComponent>();
    static int amount = 1;
    static PhysicsShapes currentShape = PhysicsShapes::eSPHERE;
    ImGui::Text("Physics Entities: %u", static_cast<unsigned int>(view.size()));
    ImGui::Text("Active bodies: %u", _physicsModule._physicsSystem->GetNumActiveBodies(JPH::EBodyType::RigidBody));

    ImGui::DragInt("Amount", &amount, 1, 1, 100);
    const char* shapeNames[] = { "Sphere", "Box", "Convex Hull" };
    const char* currentItem = shapeNames[static_cast<int>(currentShape)];

    if (ImGui::BeginCombo("Select Physics Shape", currentItem)) // Dropdown name
    {
        for (int n = 0; n < IM_ARRAYSIZE(shapeNames); n++)
        {
            bool isSelected = (static_cast<int>(currentShape) == n);
            if (ImGui::Selectable(shapeNames[n], isSelected))
            {
                currentShape = static_cast<PhysicsShapes>(n);
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus(); // Ensure the currently selected item is focused
        }
        ImGui::EndCombo();
    }

    ImGui::End();
}

void PhysicsSystem::InspectRigidBody(RigidbodyComponent& rb)
{
    _physicsModule.GetBodyInterface().ActivateBody(rb.bodyID);

    ImGui::PushID(&rb.bodyID);
    JPH::Vec3 position = _physicsModule.GetBodyInterface().GetPosition(rb.bodyID);
    float pos[3] = { position.GetX(), position.GetY(), position.GetZ() };
    if (ImGui::DragFloat3("Position", pos, 0.1f))
    {
        _physicsModule.GetBodyInterface().SetPosition(rb.bodyID, JPH::Vec3(pos[0], pos[1], pos[2]), JPH::EActivation::Activate);
    }

    const auto joltRotation = _physicsModule.GetBodyInterface().GetRotation(rb.bodyID).GetEulerAngles();
    float euler[3] = { joltRotation.GetX(), joltRotation.GetY(), joltRotation.GetZ() };
    if (ImGui::DragFloat3("Rotation", euler))
    {
        JPH::Quat newRotation = JPH::Quat::sEulerAngles(JPH::Vec3(euler[0], euler[1], euler[2]));
        _physicsModule.GetBodyInterface().SetRotation(rb.bodyID, newRotation, JPH::EActivation::Activate);
    }

    // NOTE: Disabled because of the new layer system

    // JPH::EMotionType rbType = _physicsModule.GetBodyInterface().GetMotionType(rb.bodyID);
    // const char* rbTypeNames[] = { "Static", "Kinematic", "Dynamic" };
    // const char* currentItem = rbTypeNames[static_cast<uint8_t>(rbType)];

    // if (ImGui::BeginCombo("Body type", currentItem))
    // {
    //     for (int n = 0; n < IM_ARRAYSIZE(rbTypeNames); n++)
    //     {
    //         bool isSelected = (rbType == static_cast<JPH::EMotionType>(n));
    //         if (ImGui::Selectable(rbTypeNames[n], isSelected))
    //         {
    //             _physicsModule.GetBodyInterface().SetMotionType(rb.bodyID, static_cast<JPH::EMotionType>(n), JPH::EActivation::Activate);

    //             if (static_cast<JPH::EMotionType>(n) == JPH::EMotionType::Static)
    //             {
    //                 _physicsModule.GetBodyInterface().SetObjectLayer(rb.bodyID, eNON_MOVING_OBJECT);
    //             }
    //             else
    //             {
    //                 _physicsModule.GetBodyInterface().SetObjectLayer(rb.bodyID, eMOVING_OBJECT);
    //             }
    //         }
    //         if (isSelected)
    //             ImGui::SetItemDefaultFocus(); // Ensure the currently selected item is focused
    //     }
    //     ImGui::EndCombo();
    // }

    ImGui::Text("Velocity: %f, %f, %f", _physicsModule.GetBodyInterface().GetLinearVelocity(rb.bodyID).GetX(), _physicsModule.GetBodyInterface().GetLinearVelocity(rb.bodyID).GetY(), _physicsModule.GetBodyInterface().GetLinearVelocity(rb.bodyID).GetZ());
    ImGui::Text("Speed %f", _physicsModule.GetBodyInterface().GetLinearVelocity(rb.bodyID).Length());
    ImGui::PopID();
}
