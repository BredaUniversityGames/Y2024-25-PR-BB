#pragma once

#include "module_interface.hpp"

class FMOD_SYSTEM;

class AudioModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    AudioModule();
    ~AudioModule() override = default;

    // Do fun audio stuff

private:
    FMOD_SYSTEM* _fmodSystem;
};