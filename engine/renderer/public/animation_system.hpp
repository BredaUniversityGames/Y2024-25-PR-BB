#pragma once

#include "system_interface.hpp"

#include <cstdint>

class RendererModule;

class AnimationSystem final : public SystemInterface
{
public:
    AnimationSystem(RendererModule& rendererModule);
    ~AnimationSystem() override;

    void Update(ECSModule& ecs, float dt) override;
    void Render(const ECSModule& ecs) const override;
    void Inspect() override;

private:
    RendererModule& _rendererModule;
    uint32_t _frameIndex;
};