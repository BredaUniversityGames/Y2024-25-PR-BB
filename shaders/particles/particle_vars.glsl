const uint MAX_PARTICLES = 1024 * 64;

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
    vec3 color;
    float frameRate;
    vec3 velocityRandomness;
    uint frameCount;
    ivec2 maxFrames;
    vec2 textureMultiplier;
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
    vec3 color;
    ivec2 frameOffsetCurrent;
    ivec2 frameOffsetNext;
    vec2 textureMultiplier;
};

struct CulledInstances
{
    uint count;
    ParticleInstance instances[MAX_PARTICLES];
};