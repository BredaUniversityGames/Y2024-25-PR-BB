const uint MAX_PARTICLES = 1024;

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

struct ParticleInstance
{
    vec3 position;
    uint materialIndex;
};

struct CulledInstance
{
    uint count;
    uint indices[MAX_PARTICLES];
};