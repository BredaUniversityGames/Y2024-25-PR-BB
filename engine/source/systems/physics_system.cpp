#include "systems/physics_system.hpp"
#include "ECS.hpp"

PhysicsSystem::PhysicsSystem()
{
}
PhysicsSystem::~PhysicsSystem()
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

    ImGui::End();
}