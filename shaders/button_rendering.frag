#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants
{
    mat4 MVP;
    uint TextureIndex;
} pushConstants;

void main()
{
    outColor = vec4(texture(bindless_color_textures[nonuniformEXT(pushConstants.textureindex)], uv));
}