#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform fragConsts
{
    mat4 dummy;
    uint textureindex;
} fragConstants;

void main()
{

    outColor = vec4(texture(bindless_color_textures[nonuniformEXT(fragConstants.textureindex)], uv));
}