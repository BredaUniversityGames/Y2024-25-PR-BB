#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "../bindless.glsl"
#include "particle_vars.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec2 texCoord;
layout (location = 3) flat in uint materialIndex;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(bindless_color_textures[nonuniformEXT(materialIndex)], texCoord);
}