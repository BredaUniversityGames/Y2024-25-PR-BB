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


void main()
{
    Material material;

    uint actualDrawID;

    if (pc.isDirectCommand == 1)
    {
        actualDrawID = pc.directInstanceIndex;
    }
    else
    {
        actualDrawID = drawID;
    }

    material = bindless_materials[nonuniformEXT(instances[actualDrawID].materialIndex)];

    vec4 albedoSample = vec4(1.0);
    vec4 mrSample = vec4(0.0);
    vec4 occlusionSample = vec4(0.0);
    vec4 normalSample = vec4(normalIn, 0.0);

    vec3 normal = normalIn;

    ivec2 pixelPos = ivec2(gl_FragCoord.xy);
    if (instances[actualDrawID].transparency < bayer[pixelPos.x % 4][pixelPos.y % 4] && instances[actualDrawID].transparency != 1.0)
    {
        discard;
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
