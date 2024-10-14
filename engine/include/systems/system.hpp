#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // Suppress warnings about meta deprecated
#include "entt/entt.hpp"
#pragma GCC diagnostic pop
struct SceneDescription;
class ECS;

class System
{
public:
    virtual ~System() = default;

    virtual void Update([[maybe_unused]] ECS& ecs, [[maybe_unused]] float dt) {};
    virtual void Render([[maybe_unused]] const ECS& ecs) const {};
    virtual void Inspect() {};
};