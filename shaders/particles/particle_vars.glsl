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
    uint realEmitCount;
};

const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = PARTICLECOUNTER_OFFSET_ALIVECOUNT + 4;
const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = PARTICLECOUNTER_OFFSET_DEADCOUNT + 4;

layout(set = 1, binding = 0) buffer ParticleSSBO
{
    Particle particles[ ];
};

layout(set = 1, binding = 1) buffer AliveNEWSSBO
{
    uint anIndex[ ];
};

layout(set = 1, binding = 2) buffer AliveCURRENTSSBO
{
    uint acIndex[ ];
};

layout(set = 1, binding = 3) buffer DeadSSBO
{
    uint dIndex[ ];
};

layout(set = 1, binding = 4) buffer CounterSSBO
{
    uint count[3];
};