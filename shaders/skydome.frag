#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"

layout (push_constant) uniform PushConstants
{
    uint index;
} pc;

layout (set = 2, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};

layout (location = 0) in vec2 texCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outBrightness;

void main()
{
    outColor = vec4(texCoord, 0.0, 1.0);//texture(bindless_color_textures[nonuniformEXT(pc.index)], texCoord);

    // We store brightness for bloom later on
    float brightnessStrength = dot(outColor.rgb, bloomSettings.colorWeights);
    vec3 brightnessColor = outColor.rgb * (brightnessStrength * bloomSettings.gradientStrength);
    brightnessColor = min(brightnessColor, bloomSettings.maxBrightnessExtraction);
    outBrightness = vec4(brightnessColor, 1.0);
}