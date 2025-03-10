#version 460

#include "scene.glsl"
#include "skinning.glsl"

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

layout (std430, set = 3, binding = 0) readonly buffer SkinningTransforms
{
    mat2x4 skinningTransforms[];
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;
layout (location = 4) in vec4 inJoints;
layout (location = 5) in vec4 inWeights;

layout (location = 0) out vec3 position;

mat2x4 GetJointTransform(ivec4 joints, vec4 weights, uint boneOffset)
{
    mat2x4 dq0 = skinningTransforms[joints.x + boneOffset];
    mat2x4 dq1 = skinningTransforms[joints.y + boneOffset];
    mat2x4 dq2 = skinningTransforms[joints.z + boneOffset];
    mat2x4 dq3 = skinningTransforms[joints.w + boneOffset];

    weights.y *= sign(dot(dq0[0], dq1[0]));
    weights.z *= sign(dot(dq0[0], dq2[0]));
    weights.w *= sign(dot(dq0[0], dq3[0]));

    mat2x4 result =
    weights.x * dq0 +
    weights.y * dq1 +
    weights.z * dq2 +
    weights.w * dq3;

    float norm = length(result[0]);
    return result / norm;
}

void main()
{
    const Instance instance = instances[redirect[gl_DrawID]];

    mat2x4 bone = GetJointTransform(ivec4(inJoints), inWeights, instance.boneOffset);
    mat4 skinMatrix = instance.model * GetSkinMatrix(bone);

    position = (skinMatrix * vec4(inPosition, 1.0)).xyz;

    gl_Position = (scene.directionalLight.lightVP) * vec4(position, 1.0);
}
