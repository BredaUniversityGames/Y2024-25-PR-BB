#version 460
#extension GL_EXT_nonuniform_qualifier : enable

#include "bindless.glsl"

layout(push_constant) uniform PushConstants
{
    uint index;
} pc;

layout (set = 2, binding = 0) uniform BloomSettingsUBO
{
    float strength;
    float gradientStrength;
} bloomSettings;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outBrightness;

void main()
{
    outColor = texture(bindless_color_textures[nonuniformEXT(pc.index)], texCoord);

    // We store brightness for bloom later on
    float brightness = dot(outColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    outBrightness = vec4(outColor.rgb * (brightness * bloomSettings.gradientStrength), 1.0);
}