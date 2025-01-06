#pragma once

#include "particle_util.hpp"

struct ActiveEmitterTag
{
};

struct EmitterComponent
{
    bool emitOnce = true;
    float maxEmitDelay = 1.0f;
    float currentEmitDelay = 1.0f;
    ParticleType type = ParticleType::eBillboard;
    Emitter emitter;
};