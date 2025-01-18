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

layout (location = 0) out vec4 outAlbedoM;// RGB: Albedo,   A: Metallic
layout (location = 1) out vec2 outNormal;// RG: Normal
layout (location = 2) out vec4 outPositionR;// RGB: Position  A: Roughness

layout (std430, set = 1, binding = 0) buffer InstanceData
{
    Instance instances[];
};

void main()
{
    Material material = bindless_materials[nonuniformEXT(instances[drawID].materialIndex)];

    vec4 albedoSample = vec4(1.0);
    vec4 mrSample = vec4(0.0);
    vec4 occlusionSample = vec4(0.0);
    vec4 normalSample = vec4(normalIn, 0.0);

    vec3 normal = normalIn;

    if (material.useAlbedoMap)
    {
        albedoSample = pow(texture(bindless_color_textures[nonuniformEXT(material.albedoMapIndex)], texCoord), vec4(2.2));
        if (albedoSample.a < 1.0)
        {
            discard;
        }
    }
    if (material.useMRMap)
    {
        mrSample = texture(bindless_color_textures[nonuniformEXT(material.mrMapIndex)], texCoord);
    }
    if (material.useNormalMap)
    {
        normalSample = texture(bindless_color_textures[nonuniformEXT(material.normalMapIndex)], texCoord);
    }
    if (material.useOcclusionMap)
    {
        occlusionSample = texture(bindless_color_textures[nonuniformEXT(material.occlusionMapIndex)], texCoord);
    }

    albedoSample *= pow(material.albedoFactor, vec4(2.2));
    mrSample *= vec4(1.0, material.roughnessFactor, material.metallicFactor, 1.0);
    occlusionSample *= vec4(material.occlusionStrength);

    if (material.useNormalMap)
    {
        normal = normalSample.xyz * 2.0 - 1.0;
        normal = normalize(TBN * normal) * material.normalScale;
    }

    outAlbedoM = vec4(albedoSample.rgb, mrSample.b);
    outNormal = OctEncode(normal);
    outPositionR = vec4(position, mrSample.g);
}