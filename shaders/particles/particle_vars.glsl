const uint MAX_PARTICLES = 1024 * 256;

struct Particle
{
    vec3 position;
    float mass;
    vec3 velocity;
    float maxLife;
    vec2 rotationVelocity;
    float life;
    uint materialIndex;
    vec2 size;
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
};

struct CulledInstances
{
    uint count;
    ParticleInstance instances[MAX_PARTICLES];
};