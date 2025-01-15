#version 460

#include "scene.glsl"

layout (std430, set = 1, binding = 0) buffer InstanceData
{
    Instance instances[];
};

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (set = 3, binding = 0) buffer RedirectBuffer
{
    uint count;
    uint redirect[];
};

layout (std430, set = 4, binding = 0) readonly buffer SkinningMatrices
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
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 4) out mat3 TBN;
layout (location = 3) out flat uint drawID;

void main()
{
    Instance instance = instances[redirect[gl_DrawID]];

    mat4 skinMatrix =
    inWeights.x * skinningMatrices[int(inJoints.x) + instance.boneOffset] +
    inWeights.y * skinningMatrices[int(inJoints.y) + instance.boneOffset] +
    inWeights.z * skinningMatrices[int(inJoints.z) + instance.boneOffset] +
    inWeights.w * skinningMatrices[int(inJoints.w) + instance.boneOffset];

    position = (skinMatrix * vec4(inPosition, 1.0)).xyz;
    normal = normalize(skinMatrix * vec4(inNormal, 0.0)).xyz;

    mat4 modelTransform = instance.model;
    drawID = redirect[gl_DrawID];

    vec3 tangent = normalize((modelTransform * vec4(inTangent.xyz, 0.0)).xyz);
    vec3 bitangent = normalize((modelTransform * vec4(inTangent.w * cross(inNormal, inTangent.xyz), 0.0)).xyz);
    TBN = mat3(tangent, bitangent, normal);
    texCoord = inTexCoord;

    gl_Position = (camera.VP) * vec4(position, 1.0);
    vec3 viewPos = (camera.view * vec4(position, 1.0)).xyz;
    position = viewPos;
}
