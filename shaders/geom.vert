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

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 3) out flat int drawID;
layout (location = 4) out mat3 TBN;

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
    mat4 modelTransform = instances[gl_DrawID].model;
    drawID = gl_DrawID;

    position = (modelTransform * vec4(inPosition, 1.0)).xyz;

    mat3 normalTransform = Adjoint(modelTransform);
    normal = normalize(normalTransform * inNormal);
    vec3 tangent = normalize(normalTransform * inTangent.xyz);
    vec3 bitangent = normalize(normalTransform * (inTangent.w * cross(inNormal, inTangent.xyz)));
    TBN = mat3(tangent, bitangent, normal);
    texCoord = inTexCoord;

    gl_Position = (camera.VP) * vec4(position, 1.0);
}