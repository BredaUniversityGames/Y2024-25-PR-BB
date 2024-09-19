#version 460

layout(set = 0, binding = 0) uniform UBO
{
    mat4 model;
} ubo;

layout(set = 1, binding = 0) uniform CameraUBO
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 lightVP;
    mat4 depthBiasMVP;
    vec4 lightData;
    vec3 cameraPosition;
    float _padding;
} cameraUbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec3 inColor;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 texCoord;
layout(location = 3) out mat3 TBN;

void main()
{
    position = (ubo.model * vec4(inPosition, 1.0)).xyz;
    normal = normalize((ubo.model * vec4(inNormal, 0.0)).xyz);
    vec3 tangent = normalize((ubo.model * vec4(inTangent.xyz, 0.0)).xyz);
    vec3 bitangent = normalize((ubo.model * vec4(inTangent.w * cross(inNormal, inTangent.xyz), 0.0)).xyz);
    TBN = mat3(tangent, bitangent, normal);
    texCoord = inTexCoord;

    gl_Position = (cameraUbo.VP) * vec4(position, 1.0);
}