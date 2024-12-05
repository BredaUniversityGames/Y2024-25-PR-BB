#pragma once

#include "common.hpp"
#include <memory>
#include "module_interface.hpp"

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