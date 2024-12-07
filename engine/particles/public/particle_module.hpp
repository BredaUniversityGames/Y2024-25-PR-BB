#pragma once

#include "common.hpp"
#include "module_interface.hpp"
#include "particle_interface.hpp"

#include <memory>

class Renderer;
class ParticleInterface;

class ParticleModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
    void Tick(MAYBE_UNUSED Engine& engine) override {};

public:
    ParticleModule() = default;
    ~ParticleModule() override = default;

    ParticleInterface& GetParticleInterface() { return *_particleInterface; }

private:
    std::unique_ptr<ParticleInterface> _particleInterface;
};