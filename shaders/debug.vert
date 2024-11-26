#version 460
#include "scene.glsl"

layout (set = 0, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (location = 0) in vec3 inPosition;

void main() {

    gl_Position = (camera.VP) * vec4(inPosition, 1.0);
}
