#pragma once

#include "particle_util.hpp"

struct EmitterComponent
{
    Emitter emitter;
    ParticleType type = ParticleType::eBillboard;
    uint32_t timesToEmit = 0;
};