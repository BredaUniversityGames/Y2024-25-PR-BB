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
    int deadCount;
    uint aliveCount_afterSimulation;
    uint culledCount;
};

const uint MAX_PARTICLES = 1024;

const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = 1;
const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = 2;
const uint PARTICLECOUNTER_OFFSET_CULLEDCOUNT = 3;

layout(set = 1, binding = 0) buffer ParticleSSBO
{
    Particle particles[MAX_PARTICLES];
};

layout(set = 1, binding = 1) buffer AliveNEWSSBO
{
    uint aliveBuffer_NEW[MAX_PARTICLES];
};

layout(set = 1, binding = 2) buffer AliveCURRENTSSBO
{
    uint aliveBuffer_CURRENT[MAX_PARTICLES];
};

layout(set = 1, binding = 3) buffer DeadSSBO
{
    uint deadBuffer[MAX_PARTICLES];
};

layout(set = 1, binding = 4) buffer CounterSSBO
{
    ParticleCounters particleCounters;
};