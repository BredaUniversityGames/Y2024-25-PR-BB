struct Particle
{
    vec3 position;
    float mass;
    vec3 velocity;
    float maxLife;
    vec3 rotationVelocity;
    float life;
};

struct ParticleCounters
{
    uint aliveCount;
    uint deadCount;
    uint aliveCountAfterSimulation;
    uint culledCount;
};

const uint MAX_PARTICLES = 1024;

const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = 1;
const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = 2;
const uint PARTICLECOUNTER_OFFSET_CULLEDCOUNT = 3;

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