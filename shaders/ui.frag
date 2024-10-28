#version 450
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;


layout(push_constant) uniform PushConstants
{
    mat4 mpMatrix;// 64 bytes, aligned to 16 bytes
    vec4 color;// 16 bytes, aligned to 16 bytes
    vec2 uvp1;// 8 bytes, aligned to 8 bytes
    vec2 uvp2;// 8 bytes, aligned to 8 bytes
    uint textureIndex;// 4 bytes, aligned to 4 bytes
    bool useRedAsAlpha;
} pushConstants;


void main()
{
    if(pushConstants.useRedAsAlpha)
    {
        outColor = vec4(texture(bindless_color_textures[nonuniformEXT(pushConstants.textureIndex)], uv).r) * pushConstants.color;
    }
    else
    {
        outColor = texture(bindless_color_textures[nonuniformEXT(pushConstants.textureIndex)], uv) * pushConstants.color;
    }
}
  