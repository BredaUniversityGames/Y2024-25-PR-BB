#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"
#include "tonemapping.glsl"

layout (push_constant) uniform PushConstants
{
    uint hdrTargetIndex;
    uint bloomTargetIndex;

    uint tonemappingFunction;
    float exposure;
} pc;

layout (set = 1, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};

layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

void main()
{
    vec3 hdrColor = texture(bindless_color_textures[nonuniformEXT(pc.hdrTargetIndex)], texCoords).rgb;

    vec3 bloomColor = texture(bindless_color_textures[nonuniformEXT(pc.bloomTargetIndex)], texCoords).rgb;
    hdrColor += bloomColor * bloomSettings.strength;

    vec3 mapped = vec3(1.0) - exp(-hdrColor * pc.exposure);

    switch (pc.tonemappingFunction)
    {
        case ACES: mapped = aces(mapped); break;
        case AGX: mapped = agx(mapped); break;
        case FILMIC: mapped = filmic(mapped); break;
        case LOTTES: mapped = lottes(mapped); break;
        case NEUTRAL: mapped = neutral(mapped); break;
        case REINHARD: mapped = reinhard(mapped); break;
        case REINHARD2: mapped = reinhard2(mapped); break;
        case UCHIMURA: mapped = uchimura(mapped); break;
        case UNCHARTED2: mapped = uncharted2(mapped); break;
        case UNREAL: mapped = unreal(mapped); break;
    }

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    outColor = vec4(mapped, 1.0);
}