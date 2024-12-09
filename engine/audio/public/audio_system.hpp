#pragma once

#include "common.hpp"
#include "ecs_module.hpp"
#include "system_interface.hpp"

class AudioModule;
class AudioSystem final : public SystemInterface
{
public:
    AudioSystem(ECSModule& ecs, AudioModule& audioModule);
    NON_COPYABLE(AudioSystem);
    NON_MOVABLE(AudioSystem);

    void Update(ECSModule& ecs, MAYBE_UNUSED float dt) override;
    void Render(const ECSModule& ecs) const override {};
    void Inspect() override;

private:
    ECSModule& _ecs;
    AudioModule& _audioModule;
};
