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

void main() {

    gl_Position = (cameraUbo.VP) * vec4(inPosition, 1.0);
}
