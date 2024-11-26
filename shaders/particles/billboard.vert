#version 460
#include "particle_vars.glsl"
#include "../scene.glsl"

layout(set = 1, binding = 0) buffer CulledInstancesSSB
{
    CulledInstances culledInstances;
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
    ParticleInstance instance = culledInstances.instances[gl_InstanceIndex];

    materialIndex = instance.materialIndex;
    texCoord = inTexCoord;

    position = instance.position + cameraRight * inPosition.x * instance.size.x
        + cameraUp * inPosition.y * instance.size.y;

    gl_Position = (camera.VP) * vec4(position, 1.0);
}
