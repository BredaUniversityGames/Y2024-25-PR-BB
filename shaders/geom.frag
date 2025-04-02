#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"
#include "octahedron.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in flat uint drawID;
layout (location = 4) in mat3 TBN;

layout (location = 0) out vec4 outAlbedoRM;// RGB: Albedo, A: Roughness Metallic
layout (location = 1) out vec4 outNormal;// RG: Normal

layout (std430, set = 1, binding = 0) buffer InstanceData
{
    Instance instances[];
};

layout (push_constant) uniform PushConstants
{
    uint isDirectCommand;
    uint directInstanceIndex;
} pc;

const mat4 bayer = mat4(
1.0 / 17.0, 13.0 / 17.0, 4.0 / 17.0, 16.0 / 17.0,
9.0 / 17.0, 5.0 / 17.0, 12.0 / 17.0, 8.0 / 17.0,
3.0 / 17.0, 15.0 / 17.0, 2.0 / 17.0, 14.0 / 17.0,
11.0 / 17.0, 7.0 / 17.0, 10.0 / 17.0, 6.0 / 17.0
);

// A simple 32-bit hash function
uint hash32(uint x) {
    x ^= x >> 16;
    x *= 0x7FEB352D;
    x ^= x >> 15;
    x *= 0x846CA68B;
    x ^= x >> 16;
    return x;
}

// Return a float in [0..1) from the hash
float randomFloatFromCoord(ivec2 coord)
{
    // Combine x and y into a single 32-bit integer
    // so that each pixel coordinate produces a unique seed.
    // The shift + XOR helps reduce collisions.
    uint seed = hash32(uint(coord.x) ^ (uint(coord.y) << 16));
    // Map [0 .. 2^32-1] to [0.0 .. 1.0).
    return float(seed) * (1.0 / 4294967296.0);
}

void main()
{
    Material material;

    if (pc.isDirectCommand == 1)
    {
        material = bindless_materials[nonuniformEXT(instances[pc.directInstanceIndex].materialIndex)];
    }
    else
    {
        material = bindless_materials[nonuniformEXT(instances[drawID].materialIndex)];
    }

    vec4 albedoSample = vec4(1.0);
    vec4 mrSample = vec4(0.0);
    vec4 occlusionSample = vec4(0.0);
    vec4 normalSample = vec4(normalIn, 0.0);

    vec3 normal = normalIn;

    float alpha = instances[drawID].transparency;

    // If alpha is not fully opaque (1.0), apply noise dithering
    if (alpha < 1.0)
    {
        ivec2 pixelPos = ivec2(gl_FragCoord.xy);
        float noiseVal = randomFloatFromCoord(pixelPos);

        // If the transparency is less than this random threshold, discard
        if (alpha < noiseVal)
        {
            discard;
        }
    }

    if (material.useAlbedoMap)
    {
        albedoSample = pow(texture(bindless_color_textures[nonuniformEXT (material.albedoMapIndex)], texCoord), vec4(2.2));
        if (albedoSample.a < 1.0)
        {
            discard;
        }
    }
    if (material.useMRMap)
    {
        mrSample = texture(bindless_color_textures[nonuniformEXT (material.mrMapIndex)], texCoord);
    }
    if (material.useNormalMap)
    {
        normalSample = texture(bindless_color_textures[nonuniformEXT (material.normalMapIndex)], texCoord);
    }
    if (material.useOcclusionMap)
    {
        occlusionSample = texture(bindless_color_textures[nonuniformEXT (material.occlusionMapIndex)], texCoord);
    }

    albedoSample *= pow(material.albedoFactor, vec4(2.2));
    mrSample *= vec4(1.0, material.roughnessFactor, material.metallicFactor, 1.0);
    occlusionSample *= vec4(material.occlusionStrength);

    if (material.useNormalMap)
    {
        normal = normalSample.xyz * 2.0 - 1.0;
        normal = normalize(TBN * normal) * material.normalScale;
    }

    outAlbedoRM = vec4(albedoSample.rgb, EncodeRM(clamp(mrSample.g, 0.0, 1.0), clamp(mrSample.b, 0.0, 1.0)));
    outNormal = vec4(OctEncode(normal), 0.0, 0.0);
}
