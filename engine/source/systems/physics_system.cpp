#include "systems/physics_system.hpp"
#include "ECS.hpp"
#include "modules/physics_module.hpp"
#include "components/RigidbodyComponent.hpp"

PhysicsSystem::PhysicsSystem(ECS& ecs, PhysicsModule& physicsModule)
    : _ecs(ecs)
    , _physicsModule(physicsModule)
{
}

entt::entity PhysicsSystem::CreatePhysicsEntity()
{
    entt::entity entity = _ecs._registry.create();
    RigidbodyComponent rb(*_physicsModule.body_interface);
    _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
    return entity;
}

void PhysicsSystem::CreatePhysicsEntity(RigidbodyComponent& rb)
{
    entt::entity entity = _ecs._registry.create();
    _ecs._registry.emplace<RigidbodyComponent>(entity, rb);
}

void PhysicsSystem::AddRigidBody(entt::entity entity, RigidbodyComponent& rigidbody)
{
}
void PhysicsSystem::CleanUp()
{
    const auto toDestroy = _ecs._registry.view<ECS::ToDestroy, RigidbodyComponent>();
    for (const entt::entity entity : toDestroy)
    {
        RigidbodyComponent& rb = toDestroy.get<RigidbodyComponent>(entity);
        _physicsModule.body_interface->RemoveBody(rb.bodyID);
    }
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
    static int amount = 1;
    ImGui::Text("Physics Entities: %lu", view.size());

    ImGui::DragInt("Amout", &amount, 1, 1, 100);
    if (ImGui::Button("Create Physics Entities"))
    {
        for (int i = 0; i < amount; i++)
        {
            entt::entity newEntity = CreatePhysicsEntity();
            RigidbodyComponent& rb = _ecs._registry.get<RigidbodyComponent>(newEntity);
            _physicsModule.body_interface->SetLinearVelocity(rb.bodyID, JPH::Vec3(0.6f, 0.0f, 0.0f));
        }
    }

    if (ImGui::Button("Create Plane Entity"))
    {
        JPH::BodyCreationSettings plane_settings(new JPH::BoxShape(JPH::Vec3(10.0f, 0.1f, 10.0f)), JPH::Vec3(0.0, 0.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, PhysicsLayers::NON_MOVING);

        RigidbodyComponent newRigidBody(*_physicsModule.body_interface, plane_settings);
        CreatePhysicsEntity(newRigidBody);
    }

    if (ImGui::Button("Clear Physics Entities"))
    {
        _ecs._registry.view<RigidbodyComponent>().each([&](auto entity, auto&)
            { _ecs.DestroyEntity(entity); });
    }
    ImGui::End();
}