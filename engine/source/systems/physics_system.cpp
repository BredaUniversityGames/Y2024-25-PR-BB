#include "systems/physics_system.hpp"
#include "ECS.hpp"
#include "modules/physics_module.hpp"
#include "components/RigidbodyComponent.hpp"

PhysicsSystem::PhysicsSystem(ECS& ecs, PhysicsModule& physicsModule)
    : _ecs(ecs)
    , _physicsModule(physicsModule)
{
}
PhysicsSystem::~PhysicsSystem()
{
}
void PhysicsSystem::CreatePhysicsEntity()
{
    entt::entity entity = _ecs._registry.create();
    RigidbodyComponent rb(*_physicsModule.body_interface);
    _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
}

void PhysicsSystem::CreatePhysicsEntity(RigidbodyComponent& rb)
{
    entt::entity entity = _ecs._registry.create();
    _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
}

void PhysicsSystem::AddRigidBody(entt::entity entity, RigidbodyComponent& rigidbody)
{
}
void PhysicsSystem::Update(ECS& ecs, float deltaTime)
{
}
void PhysicsSystem::Render(const ECS& ecs) const
{
}
void PhysicsSystem::Inspect()
{
    ImGui::Begin("Physics System");
    const auto view = _ecs._registry.view<RigidbodyComponent>();
    ImGui::Text("Physics Entities: %d", view.size());
    if (ImGui::Button("Create Physics Entity"))
    {
        CreatePhysicsEntity();
    }

    if (ImGui::Button("Create Plane Entity"))
    {
        JPH::BodyCreationSettings plane_settings(new JPH::BoxShape(JPH::Vec3(10.0f, 0.1f, 10.0f)), JPH::Vec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

        RigidbodyComponent newRigidBody(*_physicsModule.body_interface, plane_settings);
        CreatePhysicsEntity(newRigidBody);
    }
    ImGui::End();
}