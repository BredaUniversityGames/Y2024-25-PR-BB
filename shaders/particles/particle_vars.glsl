struct Particle
{
    float3 position;
    float mass;
    float3 force;
    float rotationalVelocity;
    float3 velocity;
    float maxLife;
    float2 sizeBeginEnd;
    float life;
    uint color;
};

struct ParticleCounters
{
    uint aliveCount;
    uint deadCount;
    uint realEmitCount;
    //uint aliveCount_afterSimulation;
    //uint culledCount;
    //uint cellAllocator;
};

const uint MAX_PARTICLES = 1024; // temporary value
const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT = 0;
const uint PARTICLECOUNTER_OFFSET_DEADCOUNT = PARTICLECOUNTER_OFFSET_ALIVECOUNT + 4;
const uint PARTICLECOUNTER_OFFSET_ALIVECOUNT_AFTERSIMULATION = PARTICLECOUNTER_OFFSET_DEADCOUNT + 4;

layout(set = 2, binding = 0) buffer