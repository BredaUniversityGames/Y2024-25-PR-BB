#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"

layout (push_constant) uniform PushConstants
{
    uint index;
    uint mip;
    vec2 resolution;
} sourceImage;

layout (location = 0) in vec2 texCoord;

// TODO: Use vec3
layout (location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texCoord, 0.0, 1.0);
    return;

    vec2 sourceTexelSize = 1.0 / sourceImage.resolution;
    float x = sourceTexelSize.x;
    float y = sourceTexelSize.y;

    // . . . . . . .
    // . A . B . C .
    // . . J . K . .
    // . D . E . F .
    // . . L . M . .
    // . G . H . I .
    // . . . . . . .
    vec3 a = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - 2 * x, texCoord.y + 2 * y), sourceImage.mip).rgb;
    vec3 b = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x,       texCoord.y + 2 * y), sourceImage.mip).rgb;
    vec3 c = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + 2 * x, texCoord.y + 2 * y), sourceImage.mip).rgb;

    vec3 d = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - 2 * x, texCoord.y), sourceImage.mip).rgb;
    vec3 e = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x,       texCoord.y), sourceImage.mip).rgb;
    vec3 f = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + 2 * x, texCoord.y), sourceImage.mip).rgb;

    vec3 g = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - 2 * x, texCoord.y - 2 * y), sourceImage.mip).rgb;
    vec3 h = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x,       texCoord.y - 2 * y), sourceImage.mip).rgb;
    vec3 i = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + 2 * x, texCoord.y - 2 * y), sourceImage.mip).rgb;

    vec3 j = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - x, texCoord.y + y), sourceImage.mip).rgb;
    vec3 k = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + x, texCoord.y + y), sourceImage.mip).rgb;
    vec3 l = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - x, texCoord.y - y), sourceImage.mip).rgb;
    vec3 m = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + x, texCoord.y - y), sourceImage.mip).rgb;

    outColor.rgb = e * 0.125;
    outColor.rgb += (a + c + g + i) * 0.03125;
    outColor.rgb += (b + d + f + h) * 0.0625;
    outColor.rgb += (j + k + l + m) * 0.125;
}
