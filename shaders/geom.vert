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

layout (push_constant) uniform PushConstants
{
    uint isDirectCommand;
    uint directInstanceIndex;
} pc;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 texCoord;
layout (location = 3) out flat uint drawID;
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
    mat4 modelTransform;

    if (pc.isDirectCommand == 1)
    {
        modelTransform = instances[pc.directInstanceIndex].model;
    }
    else
    {
        modelTransform = instances[redirect[gl_DrawID]].model;
        drawID = redirect[gl_DrawID];
    }

    position = (modelTransform * vec4(inPosition, 1.0)).xyz;

    mat3 normalTransform = Adjoint(modelTransform);
    normal = normalize(normalTransform * inNormal);
    vec3 tangent = normalize(normalTransform * inTangent.xyz);
    vec3 bitangent = normalize(normalTransform * (inTangent.w * cross(inNormal, inTangent.xyz)));
    TBN = mat3(tangent, bitangent, normal);
    texCoord = inTexCoord;

    gl_Position = (camera.VP) * vec4(position, 1.0);
    vec3 viewPos = (camera.view * vec4(position, 1.0)).xyz;
    position = viewPos; // position is in view space
}
