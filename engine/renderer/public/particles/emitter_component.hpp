#pragma once

struct EmitterComponent
{
    Emitter emitter;
    ParticleType type = ParticleType::eBillboard;
    uint32_t timesToEmit = 0;
};