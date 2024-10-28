#version 460
#include "particle_vars.glsl"
#include "../scene.glsl"

layout(set = 1, binding = 0) buffer ParticleInstancesSSB
{
    ParticleInstance particleInstances[MAX_PARTICLES];
};

layout(set = 1, binding = 1) buffer CulledInstanceSSB
{
    CulledInstance culledInstance;
};

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout(push_constant) uniform BufferOffset
{
    vec3 cameraRight;
    vec3 cameraUp;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 3) out uint materialIndex;

void main()
{
    uint particleIndex = culledInstance.indices[gl_InstanceIndex];
    ParticleInstance instance = particleInstances[particleIndex];

    materialIndex = instance.materialIndex;
    texCoord = inTexCoord;

    position = instance.position + cameraRight * inPosition.x + cameraUp * inPosition.y;

    gl_Position = (camera.VP) * vec4(position, 1.0);
}
