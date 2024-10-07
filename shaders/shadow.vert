#version 460

struct Instance
{
    mat4 model;
    uint materialIndex;
};

layout (std430, set = 0, binding = 0) buffer InstanceData
{
    Instance data[];
} instances;


layout (set = 1, binding = 0) uniform CameraUBO
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 lightVP;
    mat4 depthBiasMVP;
    mat4 skydomeMVP;
    vec4 lightData;
    vec3 cameraPosition;
    float _padding;

} cameraUbo;

layout (location = 0) in vec3 inPosition;
layout (location = 0) out vec3 position;

void main() {
    position = (instances.data[gl_DrawID].model * vec4(inPosition, 1.0)).xyz;
    gl_Position = (cameraUbo.lightVP) * vec4(position, 1.0);
}
