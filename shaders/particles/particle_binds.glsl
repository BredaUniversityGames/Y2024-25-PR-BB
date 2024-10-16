#include "particles.glsl"

layout(set = 1, binding = 0) buffer ParticleSSB
{
    Particle particles[MAX_PARTICLES];
};

layout(set = 1, binding = 1) buffer AliveNEWSSB
{
    uint aliveBufferNEW[MAX_PARTICLES];
};

layout(set = 1, binding = 2) buffer AliveCURRENTSSB
{
    uint aliveBufferCURRENT[MAX_PARTICLES];
};

layout(set = 1, binding = 3) buffer DeadSSB
{
    uint deadBuffer[MAX_PARTICLES];
};

layout(set = 1, binding = 4) buffer CounterSSB
{
    ParticleCounters particleCounters;
};