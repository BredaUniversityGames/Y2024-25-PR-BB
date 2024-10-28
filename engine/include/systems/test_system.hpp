#pragma once

#include "system.hpp"

class TestSystem : public System
{
public:
    TestSystem() = default;

    void Update(ECS& ecs, float dt) override;
};