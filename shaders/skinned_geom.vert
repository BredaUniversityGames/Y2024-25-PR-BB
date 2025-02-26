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
    mat2x4 skinningMatrices[];
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;
layout (location = 4) in vec4 inJoints; // Should be uvec4.
layout (location = 5) in vec4 inWeights;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 4) out mat3 TBN;
layout (location = 3) out flat uint drawID;

mat3 Adjoint(in mat4 m)
{
    return mat3(
    cross(m[1].xyz, m[2].xyz),
    cross(m[2].xyz, m[0].xyz),
    cross(m[0].xyz, m[1].xyz)
    );
}

void main()
{
    Instance instance = instances[redirect[gl_DrawID]];
    drawID = redirect[gl_DrawID];

    mat2x4 dq0 = skinningMatrices[int(inJoints.x)];
    mat2x4 dq1 = skinningMatrices[int(inJoints.y)];
    mat2x4 dq2 = skinningMatrices[int(inJoints.z)];
    mat2x4 dq3 = skinningMatrices[int(inJoints.w)];

    vec4 weights = inWeights;
    weights.y *= sign(dot(dq0[0], dq1[0]));
    weights.z *= sign(dot(dq0[0], dq2[0]));
    weights.w *= sign(dot(dq0[0], dq3[0]));

    mat2x4 result =
    weights.x * dq0 +
    weights.y * dq1 +
    weights.z * dq2 +
    weights.w * dq3;

    float norm = length(result[0]);
    result /= norm;

    mat4 skinMatrix =
    inWeights.x * skinningMatrices[int(inJoints.x) + instance.boneOffset] +
    inWeights.y * skinningMatrices[int(inJoints.y) + instance.boneOffset] +
    inWeights.z * skinningMatrices[int(inJoints.z) + instance.boneOffset] +
    inWeights.w * skinningMatrices[int(inJoints.w) + instance.boneOffset];

    position = (skinMatrix * vec4(inPosition, 1.0)).xyz;

    mat3 normalTransform = Adjoint(skinMatrix);
    normal = normalize(normalTransform * inNormal).xyz;
    vec3 tangent = normalize((normalTransform * inTangent.xyz).xyz);
    vec3 bitangent = normalize(normalTransform * (inTangent.w * cross(inNormal, inTangent.xyz)));
    TBN = mat3(tangent, bitangent, normal);
    texCoord = inTexCoord;

    gl_Position = (camera.VP) * vec4(position, 1.0);
    vec3 viewPos = (camera.view * vec4(position, 1.0)).xyz;
    position = viewPos;
}
