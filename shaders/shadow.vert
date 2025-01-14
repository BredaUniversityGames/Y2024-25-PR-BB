#version 460

#include "scene.glsl"

layout (std430, set = 0, binding = 0) buffer InstanceData
{
    Instance instances[];
};

layout (set = 1, binding = 0) uniform SceneUBO
{
    Scene scene;
};

layout (set = 2, binding = 0) buffer RedirectBuffer
{
    uint count;
    uint redirect[];
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 position;

void main()
{
    Instance instance = instances[redirect[gl_DrawID]];

    position = (instance.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = scene.directionalLight.lightVP * vec4(position, 1.0);
}
