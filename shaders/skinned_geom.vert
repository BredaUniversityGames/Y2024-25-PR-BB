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

layout (std430, set = 3, binding = 0) readonly buffer SkinningMatrices {
    mat4 skinningMatrices[];
};

layout (push_constant) uniform PushConstants
{
    uint instanceOffset;
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
layout (location = 3) out flat int drawID;

void main()
{
    mat4 skinMat =
    inWeights.x * skinningMatrices[int(inJoints.x)] +
    inWeights.y * skinningMatrices[int(inJoints.y)] +
    inWeights.z * skinningMatrices[int(inJoints.z)] +
    inWeights.w * skinningMatrices[int(inJoints.w)];

    mat4 modelTransform = instances[gl_DrawID + instanceOffset].model * skinMat;
    drawID = gl_DrawID + int(instanceOffset);

    position = (modelTransform * vec4(inPosition, 1.0)).xyz;
    normal = normalize((modelTransform * vec4(inNormal, 0.0)).xyz);
    vec3 tangent = normalize((modelTransform * vec4(inTangent.xyz, 0.0)).xyz);
    vec3 bitangent = normalize((modelTransform * vec4(inTangent.w * cross(inNormal, inTangent.xyz), 0.0)).xyz);
    TBN = mat3(tangent, bitangent, normal);
    texCoord = inTexCoord;

    gl_Position = (camera.VP) * vec4(position, 1.0);
}
