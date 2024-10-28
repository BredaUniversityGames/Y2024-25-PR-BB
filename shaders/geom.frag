#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec2 texCoord;
layout (location = 4) in mat3 TBN;
layout (location = 3) in flat int drawID;

layout (location = 0) out vec4 outAlbedoM;     // RGB: Albedo,   A: Metallic
layout (location = 1) out vec4 outNormalR;     // RGB: Normal,   A: Roughness
layout (location = 2) out vec4 outEmissiveAO;  // RGB: Emissive, A: AO
layout (location = 3) out vec4 outPosition;    // RGB: Position  A: unused

layout (std430, set = 1, binding = 0) buffer InstanceData
{
    Instance instances[];
};

void main()
{
    Material material = bindless_materials[nonuniformEXT(instances[drawID].materialIndex)];

    vec4 albedoSample = pow(material.albedoFactor, vec4(2.2));
    vec4 mrSample = vec4(1.0, material.roughnessFactor, material.metallicFactor, 1.0);
    vec4 occlusionSample = vec4(material.occlusionStrength);
    vec4 emissiveSample = pow(vec4(material.emissiveFactor, 0.0), vec4(2.2));

    vec3 normal = normalIn;

    if (material.useAlbedoMap)
    {
        albedoSample *= pow(texture(bindless_color_textures[nonuniformEXT(material.albedoMapIndex)], texCoord), vec4(2.2));
    }
    if (material.useMRMap)
    {
        mrSample *= texture(bindless_color_textures[nonuniformEXT(material.mrMapIndex)], texCoord);
    }
    if (material.useNormalMap)
    {
        vec4 normalSample = texture(bindless_color_textures[nonuniformEXT(material.normalMapIndex)], texCoord) * material.normalScale;
        normal = normalSample.xyz * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }
    if (material.useOcclusionMap)
    {
        occlusionSample *= texture(bindless_color_textures[nonuniformEXT(material.occlusionMapIndex)], texCoord);
    }
    if (material.useEmissiveMap)
    {
        emissiveSample *= pow(texture(bindless_color_textures[nonuniformEXT(material.emissiveMapIndex)], texCoord), vec4(2.2));
    }

    outAlbedoM = vec4(albedoSample.rgb, mrSample.b);
    outNormalR = vec4(normal, mrSample.g);
    outEmissiveAO = vec4(emissiveSample.rgb, occlusionSample.r);
    outPosition = vec4(position, 0.0);
}