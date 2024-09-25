#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"

layout(push_constant) uniform PushConstants
{
    uint index;
} pc;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(bindless_color_textures[nonuniformEXT(pc.index)], texCoord);
}