#pragma once

#include "common.hpp"
#include "systems/system.hpp"

class AudioModule;
class AudioSystem final : public System
{
public:
    AudioSystem(ECS& ecs, AudioModule& audioModule);
    NON_COPYABLE(AudioSystem);
    NON_MOVABLE(AudioSystem);

    void Update(ECS& ecs, float dt) override;
    void Render(const ECS& ecs) const override;
    void Inspect() override;

private:
    ECS& _ecs;
    AudioModule& _audioModule;
};
