#version 460
#include "particle_vars.glsl"
#include "../scene.glsl"

layout (set = 1, binding = 0) buffer CulledInstancesSSB
{
    ParticleInstance instances[MAX_PARTICLES];
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
layout (location = 3) out vec2 texCoord2;
layout (location = 4) out uint materialIndex;
layout (location = 5) out uint flags;
layout (location = 6) out vec3 outColor;
layout (location = 7) out float frameBlend;

void main()
{
    ParticleInstance instance = instances[gl_InstanceIndex];

    // rotate towards camera + simulated rotation and size
    vec3 quadPos = inPosition;
    mat2 rot = mat2(
    cos(instance.angle), -sin(instance.angle),
    sin(instance.angle), cos(instance.angle)
    );
    quadPos.xy *= rot;
    quadPos.xy *= instance.size;

    mat3 rotToCam = mat3(1.0f);

    // rotate with camera's right vector
    rotToCam[0][0] = camera.view[0][0];
    rotToCam[1][0] = camera.view[1][0];
    rotToCam[2][0] = camera.view[2][0];

    if ((instance.flags & LOCKY) != LOCKY)
    {
        // rotate with camera's up vector
        rotToCam[0][1] = camera.view[0][1];
        rotToCam[1][1] = camera.view[1][1];
        rotToCam[2][1] = camera.view[2][1];
    }

    quadPos *= mat3(rotToCam);
    position = instance.position + quadPos;

    // sprite sheet uv offsets calculation
    vec2 uv = inTexCoord;
    uv += instance.frameOffsetCurrent;
    uv *= instance.textureMultiplier;
    texCoord = uv;
    vec2 uv2 = inTexCoord;
    uv2 += instance.frameOffsetNext;
    uv2 *= instance.textureMultiplier;
    texCoord2 = uv2;

    normal = normalize((camera.view * vec4(inNormal, 0.0)).xyz);
    materialIndex = instance.materialIndex;
    flags = instance.flags;
    outColor = instance.color;
    frameBlend = instance.frameBlend;

    gl_Position = camera.VP * vec4(position, 1.0);
}
