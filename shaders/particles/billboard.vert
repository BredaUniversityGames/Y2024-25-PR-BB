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

    vec3 cameraRight = vec3(camera.view[0][0], camera.view[1][0], camera.view[2][0]);
    vec3 cameraUp = vec3(camera.view[0][1], camera.view[1][1], camera.view[2][1]);

    materialIndex = instance.materialIndex;
    texCoord = inTexCoord;

    position = instance.position + cameraRight * inPosition.x * instance.size.x
        + cameraUp * inPosition.y * instance.size.y;

    gl_Position = (camera.VP) * vec4(position, 1.0);
}
