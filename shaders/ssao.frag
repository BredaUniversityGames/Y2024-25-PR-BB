#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"


layout (push_constant) uniform PushConstants
{
    uint albedoMIndex;
    uint normalRIndex;
    uint emissiveAOIndex;
    uint positionIndex;
} pushConstants;

layout (set = 1, binding = 0) buffer SampleKernel { vec3 samples[]; } uSampleKernel;
layout (set = 1, binding = 1) buffer NoiseBuffer { vec3 noises[]; } uNoiseBuffer;
layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265359;


void main()
{
    vec4 albedoMSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.albedoMIndex)], texCoords);
    vec4 normalRSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.normalRIndex)], texCoords);
    vec4 emissiveAOSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.emissiveAOIndex)], texCoords);
    vec4 positionSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.positionIndex)], texCoords);

    vec3 albedo = albedoMSample.rgb;
    float metallic = albedoMSample.a;
    vec3 normal = normalRSample.rgb;
    vec3 position = positionSample.rgb;

    float roughness = normalRSample.a;
    vec3 emissive = emissiveAOSample.rgb;
    float ao = emissiveAOSample.a;

    outColor = vec4(uSampleKernel.samples[0],1.0);

}

