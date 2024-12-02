#pragma once

#include "module_interface.hpp"

struct FMOD_SYSTEM;
struct FMOD_STUDIO_SYSTEM;

class AudioModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    AudioModule() = default;
    ~AudioModule() override = default;

private:
    FMOD_SYSTEM* _coreSystem = nullptr;
    FMOD_STUDIO_SYSTEM* _studioSystem = nullptr;
};