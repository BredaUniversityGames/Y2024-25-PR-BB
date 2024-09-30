#pragma once

struct SceneDescription;

class System
{
public:
    virtual ~System() = default;

    virtual void Update([[maybe_unused]] SceneDescription& scene, [[maybe_unused]] float dt) {};
    virtual void Render([[maybe_unused]] const SceneDescription& scene) {};
};