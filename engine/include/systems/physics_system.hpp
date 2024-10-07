#pragma once
#include "systems/system.hpp"

class PhysicsSystem : public System
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    void Update([[maybe_unused]] ECS& ecs, [[maybe_unused]] float deltaTime) override;
    void Render([[maybe_unused]] const ECS& ecs) const override;
    void Inspect() override;
};