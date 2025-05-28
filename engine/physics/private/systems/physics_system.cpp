
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
    // This part should be fast because it returns a vector of just ids not whole rigidbodies
    JPH::BodyIDVector activeBodies;
    _physicsModule._physicsSystem->GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);

    // Update transform for all active bodies
    for (auto active_body : activeBodies)
    {
        if (_physicsModule.GetBodyInterface().GetMotionType(active_body) == JPH::EMotionType::Static)
        {
            bblog::trace("Skipping static body");
            return;
        }

        const entt::entity entity = static_cast<entt::entity>(_physicsModule.GetBodyInterface().GetUserData(active_body));
        const RigidbodyComponent& rb = ecs.GetRegistry().get<RigidbodyComponent>(entity);

        auto joltMatrix = _physicsModule.GetBodyInterface().GetWorldTransform(rb.bodyID);

        // We cant support objects simulated by jolt and our hierarchy system at the same time
        RelationshipComponent* relationship = _ecs.GetRegistry().try_get<RelationshipComponent>(entity);

        if (relationship && relationship->parent != entt::null)
            RelationshipHelpers::DetachChild(_ecs.GetRegistry(), relationship->parent, entity);

        // Crazy jolt stuff that I dont like but it works to set the proper scale
        auto shape = _physicsModule._physicsSystem->GetBodyInterface().GetShape(active_body);

        if (auto scaled_shape = dynamic_cast<const JPH::ScaledShape*>(shape.GetPtr()))
        {
            const auto joltScale = scaled_shape->GetScale();
            joltMatrix = joltMatrix.PreScaled(joltScale);
        }

        const auto joltToGlm = ToGLMMat4(joltMatrix);
        TransformHelpers::SetWorldTransform(ecs.GetRegistry(), entity, joltToGlm);
    }
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
