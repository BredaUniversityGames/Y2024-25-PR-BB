#include "systems/physics_system.hpp"
#include "ECS.hpp"
#include "components/name_component.hpp"
#include "modules/physics_module.hpp"
#include "components/rigidbody_component.hpp"
#include "imgui/imgui.h"

PhysicsSystem::PhysicsSystem(ECS& ecs, PhysicsModule& physicsModule)
    : _ecs(ecs)
    , _physicsModule(physicsModule)
{
}

entt::entity PhysicsSystem::CreatePhysicsEntity()
{
    entt::entity entity = _ecs._registry.create();
    RigidbodyComponent rb(*_physicsModule.bodyInterface);
    _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
    return entity;
}

void PhysicsSystem::CreatePhysicsEntity(RigidbodyComponent& rb)
{
    entt::entity entity = _ecs._registry.create();
    _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
}

void PhysicsSystem::AddRigidBody(MAYBE_UNUSED entt::entity entity, MAYBE_UNUSED RigidbodyComponent& rigidbody)
{
}
void PhysicsSystem::CleanUp()
{
    const auto toDestroy = _ecs._registry.view<ECS::ToDestroy, RigidbodyComponent>();
    for (const entt::entity entity : toDestroy)
    {
        RigidbodyComponent& rb = toDestroy.get<RigidbodyComponent>(entity);
        _physicsModule.bodyInterface->RemoveBody(rb.bodyID);
        _physicsModule.bodyInterface->DestroyBody(rb.bodyID);
    }
}

void PhysicsSystem::Update(MAYBE_UNUSED ECS& ecs, MAYBE_UNUSED float deltaTime)
{
}
void PhysicsSystem::Render(MAYBE_UNUSED const ECS& ecs) const
{
}
void PhysicsSystem::Inspect()
{
    ImGui::Begin("Physics System");
    const auto view = _ecs._registry.view<RigidbodyComponent>();
    static int amount = 1;
    static PhysicsShapes currentShape = eSPHERE;
    ImGui::Text("Physics Entities: %lu", static_cast<unsigned int>(view.size()));

    ImGui::DragInt("Amout", &amount, 1, 1, 100);
    const char* shapeNames[] = { "Sphere", "Box", "Convex Hull" };
    const char* currentItem = shapeNames[currentShape];

    if (ImGui::BeginCombo("Select Physics Shape", currentItem)) // Dropdown name
    {
        for (int n = 0; n < IM_ARRAYSIZE(shapeNames); n++)
        {
            bool isSelected = (currentShape == n);
            if (ImGui::Selectable(shapeNames[n], isSelected))
            {
                currentShape = static_cast<PhysicsShapes>(n);
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus(); // Ensure the currently selected item is focused
        }
        ImGui::EndCombo();
    }
    if (ImGui::Button("Create Physics Entities"))
    {
        for (int i = 0; i < amount; i++)
        {
            entt::entity entity = _ecs._registry.create();
            RigidbodyComponent rb(*_physicsModule.bodyInterface, currentShape);
            NameComponent node;
            node._name = "Physics Entity";
            _ecs._registry.emplace<NameComponent>(entity, node);
            _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
            _physicsModule.bodyInterface->SetLinearVelocity(rb.bodyID, JPH::Vec3(0.6f, 0.0f, 0.0f));
        }
    }

    if (ImGui::Button("Create Plane Entity"))
    {
        JPH::BodyCreationSettings plane_settings(new JPH::BoxShape(JPH::Vec3(10.0f, 0.1f, 10.0f)), JPH::Vec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);

        RigidbodyComponent newRigidBody(*_physicsModule.bodyInterface, plane_settings);
        CreatePhysicsEntity(newRigidBody);
    }

    if (ImGui::Button("Clear Physics Entities"))
    {
        _ecs._registry.view<RigidbodyComponent>().each([&](auto entity, auto&)
            { _ecs.DestroyEntity(entity); });
    }
    ImGui::End();
}

void PhysicsSystem::InspectRigidBody(RigidbodyComponent& rb)
{
    ImGui::Text("Body ID: %lu", rb.bodyID);

    JPH::Vec3 position = _physicsModule.bodyInterface->GetPosition(rb.bodyID);
    float pos[3] = { position.GetX(), position.GetY(), position.GetZ() };
    if (ImGui::DragFloat3("Position", pos, 0.1f))
    {
        _physicsModule.bodyInterface->SetPosition(rb.bodyID, JPH::Vec3(pos[0], pos[1], pos[2]), JPH::EActivation::Activate);
    }

    const auto joltRotation = _physicsModule.bodyInterface->GetRotation(rb.bodyID).GetEulerAngles();
    float euler[3] = { joltRotation.GetX(), joltRotation.GetY(), joltRotation.GetZ() };
    if (ImGui::DragFloat3("Rotation", euler))
    {
        JPH::Quat newRotation = JPH::Quat::sEulerAngles(JPH::Vec3(euler[0], euler[1], euler[2]));
        _physicsModule.bodyInterface->SetRotation(rb.bodyID, newRotation, JPH::EActivation::Activate);
    }

    if (rb.shapeType == PhysicsShapes::eSPHERE)
    {
        const auto& shape = _physicsModule.bodyInterface->GetShape(rb.bodyID);
        float radius = shape->GetInnerRadius();

        if (ImGui::DragFloat("Radius", &radius, 0.1f))
        {
            radius = glm::max(radius, 0.01f);
            _physicsModule.bodyInterface->SetShape(rb.bodyID, new JPH::SphereShape(radius), true, JPH::EActivation::Activate);
        }
    }

    if (rb.shapeType == PhysicsShapes::eBOX)
    {
        const auto& shape = _physicsModule.bodyInterface->GetShape(rb.bodyID);
        auto boxShape = JPH::StaticCast<JPH::BoxShape>(shape);
        if (boxShape == nullptr)
        {
            return;
        }
        const auto joltSize = boxShape->GetHalfExtent();
        float size[3] = { joltSize.GetX(), joltSize.GetY(), joltSize.GetZ() };
        if (ImGui::DragFloat3("Size", size, 0.1f))
        {
            size[0] = glm::max(size[0], 0.1f);
            size[1] = glm::max(size[1], 0.1f);
            size[2] = glm::max(size[2], 0.1f);
            _physicsModule.bodyInterface->SetShape(rb.bodyID, new JPH::BoxShape(JPH::Vec3(size[0], size[1], size[2])), true, JPH::EActivation::Activate);
        }
    }

    JPH::EMotionType rbType = _physicsModule.bodyInterface->GetMotionType(rb.bodyID);
    const char* rbTypeNames[] = { "Static", "Kinematic", "Dynamic" };
    const char* currentItem = rbTypeNames[static_cast<uint8_t>(rbType)];

    if (ImGui::BeginCombo("Body type", currentItem))
    {
        for (uint8_t n = 0; n < IM_ARRAYSIZE(rbTypeNames); n++)
        {
            bool isSelected = (rbType == static_cast<JPH::EMotionType>(n));
            if (ImGui::Selectable(rbTypeNames[n], isSelected))
            {
                _physicsModule.bodyInterface->SetMotionType(rb.bodyID, static_cast<JPH::EMotionType>(n), JPH::EActivation::Activate);

                if (static_cast<JPH::EMotionType>(n) == JPH::EMotionType::Static)
                {
                    _physicsModule.bodyInterface->SetObjectLayer(rb.bodyID, PhysicsLayers::NON_MOVING);
                }
                else
                {
                    _physicsModule.bodyInterface->SetObjectLayer(rb.bodyID, PhysicsLayers::MOVING);
                }
            }
            if (isSelected)
                ImGui::SetItemDefaultFocus(); // Ensure the currently selected item is focused
        }
        ImGui::EndCombo();
    }
}