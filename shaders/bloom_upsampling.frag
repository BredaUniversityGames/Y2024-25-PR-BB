#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"

layout (push_constant) uniform PushConstants
{
    uint srcImageIndex;
    uint srcImageMip;
    float filterRadius;
} pushConstants;

layout (location = 0) in vec2 texCoord;

// TODO: Use vec3
layout (location = 0) out vec4 outColor;

void main()
{
    float x = pushConstants.filterRadius;
    float y = pushConstants.filterRadius;

    // Take 9 samples around current texel:
    // . . . . . . .
    // . A . B . C .
    // . D . E . F .
    // . G . H . I .
    // . . . . . . .
    vec3 a = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x - x, texCoord.y + y), pushConstants.srcImageMip).rgb;
    vec3 b = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x,     texCoord.y + y), pushConstants.srcImageMip).rgb;
    vec3 c = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x + x, texCoord.y + y), pushConstants.srcImageMip).rgb;

    vec3 d = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x - x, texCoord.y), pushConstants.srcImageMip).rgb;
    vec3 e = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x,     texCoord.y), pushConstants.srcImageMip).rgb;
    vec3 f = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x + x, texCoord.y), pushConstants.srcImageMip).rgb;

    vec3 g = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x - x, texCoord.y - y), pushConstants.srcImageMip).rgb;
    vec3 h = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x,     texCoord.y - y), pushConstants.srcImageMip).rgb;
    vec3 i = textureLod(bindless_color_textures[nonuniformEXT(pushConstants.srcImageIndex)], vec2(texCoord.x + x, texCoord.y - y), pushConstants.srcImageMip).rgb;

    // 3x3 tent filter
    outColor.rgb = e * 4.0;
    outColor.rgb += (b + d + f + h) * 2.0;
    outColor.rgb += (a + c + g + i);
    outColor.rgb *= 1.0 / 16.0;
}
