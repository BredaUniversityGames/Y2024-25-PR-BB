#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(0.0, 1.0, 0.0, 1.0); // Green color
}