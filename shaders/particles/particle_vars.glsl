const uint MAX_PARTICLES = 1024 * 256;

// particle rendering flags
const uint UNLIT = 1 << 0;
const uint NOSHADOW = 1 << 1;

struct Particle
{
    vec3 position;
    float mass;
    vec3 velocity;
    float maxLife;
    vec2 rotationVelocity;
    float life;
    uint materialIndex;
    vec3 size;
    uint flags;
};

struct ParticleCounters
{
    uint aliveCount;
    uint deadCount;
    uint aliveCountAfterSimulation;
};

struct ParticleInstance
{
    vec3 position;
    uint materialIndex;
    vec2 size;
    float angle;
    uint flags;
};

struct CulledInstances
{
    uint count;
    ParticleInstance instances[MAX_PARTICLES];
};