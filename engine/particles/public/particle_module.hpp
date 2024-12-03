#pragma once

#include "module_interface.hpp"

class Renderer;
class ParticleInterface;

class ParticleModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    ParticleModule() = default;
    ~ParticleModule() override = default;

    ParticleInterface& GetParticleInterface() { return *_particleInterface; }

private:
    std::unique_ptr<ParticleInterface> _particleInterface;
};