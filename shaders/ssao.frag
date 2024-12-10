#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"

layout (push_constant) uniform PushConstants
{
    uint normalRIndex;
    uint positionIndex;
    uint noiseIndex;
    float aoStrength;
    float aoBias;
    float aoRadius;
} pushConstants;

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (set = 1, binding = 0) buffer SampleKernel { vec4 samples[]; } uSampleKernel;

layout (location = 0) in vec2 texCoords;
layout (location = 0) out vec4 outColor;


const vec2 noiseScale = vec2(1920.0 / 4.0, 1080.0 / 4.0); // screen = 800x600
const int kernelSize = 64;
void main()
{
    const vec4 normalRSample = texture(bindless_color_textures[nonuniformEXT (pushConstants.normalRIndex)], texCoords);
    const vec4 positionSample = texture(bindless_color_textures[nonuniformEXT (pushConstants.positionIndex)], texCoords);

    const vec3 normal = normalRSample.rgb;
    const vec3 position = positionSample.rgb;

    const vec3 screenSpacePosition = (camera.view * vec4(position.xyz, 1.0)).xyz;
    const vec3 screenSpaceNormals = (camera.view * vec4(normal.xyz, 0.0)).xyz;

    const vec3 randomVec = texture(bindless_color_textures[nonuniformEXT (pushConstants.noiseIndex)], texCoords * noiseScale).xyz;


    const vec3 tangent = normalize(randomVec - screenSpaceNormals * dot(randomVec, screenSpaceNormals));
    const vec3 bitangent = cross(screenSpaceNormals, tangent);
    const mat3 TBN = mat3(tangent, bitangent, screenSpaceNormals);

    float distanceToCamera = length(screenSpacePosition);

    // Define distance thresholds for interpolation
    const float minDistance = 0.5; // Distance at which AO parameters start to adjust
    const float maxDistance = 1.0; // Distance at which AO parameters are at full strength
    const float factor = smoothstep(minDistance, maxDistance, distanceToCamera);

    const float adaptiveAoStrength = mix(pushConstants.aoStrength * 0.25, pushConstants.aoStrength, factor);
    const float adaptiveAoRadius = mix(pushConstants.aoRadius * 0.25, pushConstants.aoRadius, factor);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; i++)
    {
        //sample pos
        const vec3 kernelResult = uSampleKernel.samples[i].xyz;
        vec3 thisSample = TBN * kernelResult;
        thisSample = screenSpacePosition + thisSample * adaptiveAoRadius;

        vec4 offset = vec4(thisSample, 1.0);
        offset = camera.proj * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        if (offset.x < 0.0 || offset.x > 1.0 || offset.y < 0.0 || offset.y > 1.0) {
            continue;
        }

        vec3 occluderPosition = texture(bindless_color_textures[nonuniformEXT (pushConstants.positionIndex)], offset.xy).xyz;
        occluderPosition = (camera.view * vec4(occluderPosition.xyz, 1.0)).xyz; // convert to view space

        //check distance to avoid ao on objects that are far away from each other
        const float rangeCheck = smoothstep(0.0, 1.0, adaptiveAoRadius / length(screenSpacePosition - occluderPosition));

        occlusion += (occluderPosition.z >= thisSample.z + pushConstants.aoBias ? 1.0 : 0.0) * rangeCheck;

    }
    const vec3 result = vec3(pow(1.0 - occlusion / float(kernelSize), adaptiveAoStrength));
    outColor = vec4(result.xyz, 1.0);

}

