#pragma once

struct EmitterComponent
{
    Emitter emitter;
    ParticleType type = ParticleType::eBillboard;
    uint32_t lifetime = 0;
};