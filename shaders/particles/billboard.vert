#version 460
#include "particle_vars.glsl"
#include "../scene.glsl"

layout (set = 1, binding = 0) buffer ParticleInstancesSSB
{
    ParticleInstance particleInstances[MAX_PARTICLES];
};

layout (set = 1, binding = 1) buffer CulledInstanceSSB
{
    CulledInstance culledInstance;
};

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;

void main()
{
    gl_Position = vec4(vec3(0.0), 1.0);
}
