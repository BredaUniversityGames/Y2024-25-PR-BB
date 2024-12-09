#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"

layout (push_constant) uniform PushConstants
{
    uint albedoMIndex;
    uint normalRIndex;
    uint emissiveAOIndex;
    uint positionIndex;
    uint noiseIndex;
} pushConstants;

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (set = 1, binding = 0) buffer SampleKernel { vec4 samples[]; } uSampleKernel;
layout (set = 1, binding = 1) buffer NoiseBuffer { vec4 noises[]; } uNoiseBuffer;
layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}


const vec2 noiseScale = vec2(1920.0 / 4.0, 1080.0 / 4.0); // screen = 800x600
const float radius = 0.5;
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

    vec3 screenSpacePosition = (camera.view * vec4(position.xyz, 1.0)).xyz;
    vec3 screenSpaceNormals = (camera.view * vec4(normal.xyz, 1.0)).xyz;

    vec3 randomVec = texture(bindless_color_textures[nonuniformEXT(pushConstants.noiseIndex)], texCoords * noiseScale).xyz;


    vec3 tangent = normalize(randomVec - screenSpaceNormals * dot(randomVec, screenSpaceNormals));
    vec3 bitangent = cross(screenSpaceNormals, tangent);
    mat3 TBN = mat3(tangent, bitangent, screenSpaceNormals);

    float occlusion = 0.0;
    for (int i = 0; i < 64; i++)
    {
        //sample pos
        vec3 kernelResult = uSampleKernel.samples[i].xyz;
        vec3 thisSample = TBN * kernelResult;
        thisSample = screenSpacePosition + thisSample * radius;

        vec4 offset = vec4(thisSample, 1.0);
        offset = camera.proj * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        vec3 occluderPosition = texture(bindless_color_textures[nonuniformEXT (pushConstants.positionIndex)], offset.xy).xyz;
        occluderPosition = (camera.view * vec4(occluderPosition.xyz, 1.0)).xyz; // convert to view space

        //check distance to avoid ao on objects that are far away from each other
        float rangeCheck = smoothstep(0.0, 1.0, radius / length(screenSpacePosition - occluderPosition));

        occlusion += (occluderPosition.z >= thisSample.z + 0.025 ? 1.0 : 0.0) * rangeCheck;

    }
    vec3 result = vec3(1.0 - occlusion / 64.0);
    outColor = vec4(result.xyz, 1.0);

}

