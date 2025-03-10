#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"
#include "settings.glsl"

layout (push_constant) uniform PushConstants
{
    uint index;
    uint mip;
} sourceImage;

layout (set = 1, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;

void main()
{
    float x = bloomSettings.filterRadius;
    float y = bloomSettings.filterRadius;

    // Take 9 samples around current texel:
    // . . . . . . .
    // . A . B . C .
    // . D . E . F .
    // . G . H . I .
    // . . . . . . .
    vec3 a = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - x, texCoord.y + y), sourceImage.mip).rgb;
    vec3 b = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x,     texCoord.y + y), sourceImage.mip).rgb;
    vec3 c = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + x, texCoord.y + y), sourceImage.mip).rgb;

    vec3 d = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - x, texCoord.y), sourceImage.mip).rgb;
    vec3 e = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x,     texCoord.y), sourceImage.mip).rgb;
    vec3 f = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + x, texCoord.y), sourceImage.mip).rgb;

    vec3 g = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x - x, texCoord.y - y), sourceImage.mip).rgb;
    vec3 h = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x,     texCoord.y - y), sourceImage.mip).rgb;
    vec3 i = textureLod(bindless_color_textures[nonuniformEXT(sourceImage.index)], vec2(texCoord.x + x, texCoord.y - y), sourceImage.mip).rgb;

    // 3x3 tent filter
    outColor.rgb = e * 4.0;
    outColor.rgb += (b + d + f + h) * 2.0;
    outColor.rgb += (a + c + g + i);
    outColor.rgb *= 1.0 / 16.0;
}
