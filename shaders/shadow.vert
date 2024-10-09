#version 460

#include "scene.glsl"

layout (std430, set = 0, binding = 0) buffer InstanceData
{
    Instance data[];
} instances;

layout (set = 1, binding = 0) uniform SceneUBO
{
    Scene scene;
};

layout (location = 0) in vec3 inPosition;
layout (location = 0) out vec3 position;

void main() {
    position = (instances.data[gl_DrawID].model * vec4(inPosition, 1.0)).xyz;
    gl_Position = (scene.directionalLight.lightVP) * vec4(position, 1.0);
}
