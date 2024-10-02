struct Particle
{
    vec3 position;
    float mass;
    vec3 velocity;
    float maxLife;
    float life;
};

struct ParticleCounters
{
    uint aliveCount;
    uint deadCount;
    uint aliveCount_afterSimulation;
    uint culledCount;
};

const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = 1;
const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = 2;
const uint PARTICLECOUNTER_OFFSET_CULLEDCOUNT = 3;

layout(set = 1, binding = 0) buffer ParticleSSBO
{
    Particle particles[ ];
};

layout(set = 1, binding = 1) buffer AliveNEWSSBO
{
    uint aliveNEWIndex[ ];
};

layout(set = 1, binding = 2) buffer AliveCURRENTSSBO
{
    uint aliveCURRENTIndex[ ];
};

layout(set = 1, binding = 3) buffer DeadSSBO
{
    uint deadIndex[ ];
};

layout(set = 1, binding = 4) buffer CounterSSBO
{
    uint particleCounters[4];
};