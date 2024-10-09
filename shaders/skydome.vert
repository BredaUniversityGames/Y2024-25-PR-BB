#version 460

#include "camera.glsl"

layout (set = 1, binding = 0) uniform CameraUBO
{
    Camera camera;
} cameraUbo;


layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec2 texCoord;

void main()
{
    texCoord = inTexCoord;

    mat4 transform = cameraUbo.camera.view;

    transform = cameraUbo.camera.proj * transform;

    gl_Position = cameraUbo.camera.skydomeMVP * vec4(inPosition, 1.0);
}