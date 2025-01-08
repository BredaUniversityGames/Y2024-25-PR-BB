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

layout (std430, set = 3, binding = 0) readonly buffer SkinningMatrices
{
    mat4 skinningMatrices[];
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;
layout (location = 4) in vec4 inJoints;
layout (location = 5) in vec4 inWeights;

layout (location = 0) out vec3 position;

void main()
{
    Instance instance = instances[redirect[gl_DrawID]];

    mat4 skinMatrix =
    inWeights.x * skinningMatrices[int(inJoints.x) + instance.boneOffset] +
    inWeights.y * skinningMatrices[int(inJoints.y) + instance.boneOffset] +
    inWeights.z * skinningMatrices[int(inJoints.z) + instance.boneOffset] +
    inWeights.w * skinningMatrices[int(inJoints.w) + instance.boneOffset];

    position = (skinMatrix * vec4(inPosition, 1.0)).xyz;

    gl_Position = (scene.directionalLight.lightVP) * vec4(position, 1.0);
}
