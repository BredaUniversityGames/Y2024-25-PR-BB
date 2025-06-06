#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"
#include "octahedron.glsl"

layout (push_constant) uniform PushConstants
{
    uint normalIndex;
    uint depthIndex;
    uint noiseIndex;
    uint ssaoRenderTargetWidth;
    uint ssaoRenderTargetHeight;
    float aoStrength;
    float aoBias;
    float aoRadius;
    float minAoDistance; // Distance at which AO parameters start to adjust
    float maxAoDistance; // Distance at which AO parameters are at full strength
} pushConstants;

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (set = 1, binding = 0) uniform SampleKernel { vec4 samples[32]; } uSampleKernel;

layout (location = 0) in vec2 texCoords;
layout (location = 0) out vec4 outColor;

const int kernelSize = 32;

void main()
{
    const vec2 noiseScale = vec2(pushConstants.ssaoRenderTargetWidth / 4.0, pushConstants.ssaoRenderTargetHeight / 4.0); // scale noise to screen size

    const vec4 normalSample = texture(bindless_color_textures[nonuniformEXT (pushConstants.normalIndex)], texCoords);
    const float depthSample = texture(bindless_depth_textures[nonuniformEXT (pushConstants.depthIndex)], texCoords).r;
    const vec3 screenSpacePosition = ReconstructViewPosition(depthSample, texCoords, camera.inverseProj);

    const vec3 normal = OctDecode(normalSample.rg);

    const vec3 screenSpaceNormals = (camera.view * vec4(normal, 0.0)).xyz;

    const vec3 randomVec = texture(bindless_color_textures[nonuniformEXT (pushConstants.noiseIndex)], texCoords * noiseScale).xyz;


    const vec3 tangent = normalize(randomVec - screenSpaceNormals * dot(randomVec, screenSpaceNormals));
    const vec3 bitangent = cross(screenSpaceNormals, tangent);
    const mat3 TBN = mat3(tangent, bitangent, screenSpaceNormals);

    float distanceToCamera = length(screenSpacePosition);

    // Treshhold factor to determine AO amount
    // Nice for performance and quality balance
    const float factor = smoothstep(pushConstants.minAoDistance, pushConstants.maxAoDistance, distanceToCamera);

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

        const float depthSample = texture(bindless_depth_textures[nonuniformEXT (pushConstants.depthIndex)], offset.xy).r;
        const vec3 occluderPosition = ReconstructViewPosition(depthSample, texCoords, camera.inverseProj);

        //check distance to avoid ao on objects that are far away from each other
        const float rangeCheck = smoothstep(0.0, 1.0, adaptiveAoRadius / length(screenSpacePosition - occluderPosition));

        occlusion += (occluderPosition.z >= thisSample.z + pushConstants.aoBias ? 1.0 : 0.0) * rangeCheck;

    }
    const vec3 result = vec3(pow(1.0 - occlusion / float(kernelSize), adaptiveAoStrength));
    outColor = vec4(result.xyz, 1.0);

}

