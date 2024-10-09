#version 460

layout (set = 1, binding = 0) uniform CameraUBO
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 skydomeMVP;
    vec3 cameraPosition;
    float _padding;
} cameraUbo;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec2 texCoord;

void main()
{
    texCoord = inTexCoord;

    mat4 transform = cameraUbo.view;

    transform = cameraUbo.proj * transform;

    gl_Position = cameraUbo.skydomeMVP * vec4(inPosition, 1.0);
}