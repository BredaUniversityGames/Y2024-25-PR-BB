#version 450
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

struct QuadDrawInfo
{
    mat4 mpMatrix;       // 64 bytes, aligned to 16 bytes
    vec4 color;          // 16 bytes, aligned to 16 bytes
    vec2 uvMin;           // 8 bytes, aligned to 8 bytes
    vec2 uvMax;           // 8 bytes, aligned to 8 bytes
    uint textureIndex;   // 4 bytes, aligned to 4 bytes
    bool useRedAsAlpha;
};

layout (push_constant) uniform PushConstants
{
    QuadDrawInfo quad;
} pushConstants;

void main()
{
    if (pushConstants.quad.useRedAsAlpha)
    {
        outColor = vec4(texture(bindless_color_textures[nonuniformEXT(pushConstants.quad.textureIndex)], uv).r) * pushConstants.quad.color;
    }
    else
    {
        outColor = texture(bindless_color_textures[nonuniformEXT(pushConstants.quad.textureIndex)], uv) * pushConstants.quad.color;
    }
}
  