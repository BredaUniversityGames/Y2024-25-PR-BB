#pragma once

#include "common.hpp"
#include "system_interface.hpp"

#include "physics_module.hpp"

class AudioModule;
class AudioSystem final : public SystemInterface
{
public:
    AudioSystem(ECSModule& ecs, AudioModule& audioModule);
    ~AudioSystem() override = default;
    NON_COPYABLE(AudioSystem);
    NON_MOVABLE(AudioSystem);

    void Update(ECSModule& ecs, MAYBE_UNUSED float dt) override;
    void Render(MAYBE_UNUSED const ECSModule& ecs) const override { }
    void Inspect() override;

    std::string_view GetName() override { return "AudioSystem"; }

private:
    MAYBE_UNUSED ECSModule& _ecs;
    AudioModule& _audioModule;
};
