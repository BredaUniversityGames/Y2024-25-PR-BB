#version 460
#include "particle_vars.glsl"
#include "../scene.glsl"

layout (set = 1, binding = 0) buffer CulledInstancesSSB
{
    CulledInstances culledInstances;
};

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 3) out uint materialIndex;
layout (location = 4) out uint flags;

void main()
{
    ParticleInstance instance = culledInstances.instances[gl_InstanceIndex];

    vec3 quadPos = inPosition;
    mat2 rot = mat2(
    cos(instance.angle), -sin(instance.angle),
    sin(instance.angle), cos(instance.angle)
    );
    quadPos.xy *= rot;
    quadPos.xy *= instance.size;
    quadPos *= mat3(camera.view);
    position = instance.position + quadPos;

    normal = normalize((camera.view * vec4(inNormal, 0.0)).xyz);
    materialIndex = instance.materialIndex;
    texCoord = inTexCoord;
    flags = instance.flags;

    gl_Position = camera.VP * vec4(position, 1.0);
}
