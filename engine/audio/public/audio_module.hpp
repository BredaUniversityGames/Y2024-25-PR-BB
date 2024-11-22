#pragma once

#include "module_interface.hpp"

#include "fmod.hpp"
#include <memory>

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
    std::unique_ptr<FMOD::System> system;
};