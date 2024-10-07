#pragma once

struct SceneDescription;
class ECS;

class System
{
public:
    virtual ~System() = default;

    virtual void Update([[maybe_unused]] ECS& ecs, [[maybe_unused]] float dt) {};
    virtual void Render([[maybe_unused]] const ECS& ecs) const {};
};