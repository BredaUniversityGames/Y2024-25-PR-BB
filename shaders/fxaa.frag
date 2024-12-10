#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"

layout (push_constant) uniform PushConstants
{
    uint sourceIndex;
} pushConstants;

layout (location = 0) in vec2 texCoords;
layout (location = 0) out vec4 outColor;


void main()
{
    vec3 sourceColor = texture(bindless_color_textures[nonuniformEXT(pushConstants.sourceIndex)], texCoords).rgb;
    outColor = vec4(sourceColor.xyz, 1.0);

}

