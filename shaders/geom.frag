#version 460
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 outAlbedoM;    // RGB: Albedo,   A: Metallic
layout(location = 1) out vec4 outNormalR;    // RGB: Normal,   A: Roughness
layout(location = 2) out vec4 outEmissiveAO; // RGB: Emissive, A: AO
layout(location = 3) out vec4 outPosition;   // RGB: Position, A: Unused

layout(set = 2, binding = 0) uniform MaterialInfoUBO
{
    vec4 albedoFactor;

    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;

    vec3 emissiveFactor;
    bool useEmissiveMap;

    bool useAlbedoMap;
    bool useMRMap;
    bool useNormalMap;
    bool useOcclusionMap;
    int albedoMapIndex;
    int mrMapIndex;
    int normalMapIndex;
    int occlusionMapIndex;
    int emissiveMapIndex;
} materialInfoUBO;

layout(set = 3, binding = 10) uniform sampler2D global_textures[];

void main()
{
    vec4 albedoSample = pow(materialInfoUBO.albedoFactor, vec4(2.2));
    vec4 mrSample = vec4(materialInfoUBO.metallicFactor, materialInfoUBO.metallicFactor, 1.0, 1.0);
    vec4 occlusionSample = vec4(materialInfoUBO.occlusionStrength);
    vec4 emissiveSample = pow(vec4(materialInfoUBO.emissiveFactor, 0.0), vec4(2.2));

    vec3 normal = normalIn;

    if(materialInfoUBO.useAlbedoMap)
    {
        albedoSample *= pow(texture(global_textures[nonuniformEXT(materialInfoUBO.albedoMapIndex)], texCoord), vec4(2.2));
    }
    if(materialInfoUBO.useMRMap)
    {
        mrSample *= texture(global_textures[nonuniformEXT(materialInfoUBO.mrMapIndex)], texCoord);
    }
    if(materialInfoUBO.useNormalMap)
    {
        vec4 normalSample = texture(global_textures[nonuniformEXT(materialInfoUBO.normalMapIndex)], texCoord) * materialInfoUBO.normalScale;
        normal = normalSample.xyz * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }
    if(materialInfoUBO.useOcclusionMap)
    {
        occlusionSample *= texture(global_textures[nonuniformEXT(materialInfoUBO.occlusionMapIndex)], texCoord);
    }
    if(materialInfoUBO.useEmissiveMap)
    {
        emissiveSample *= pow(texture(global_textures[nonuniformEXT(materialInfoUBO.emissiveMapIndex)], texCoord), vec4(2.2));
    }

    outAlbedoM = vec4(albedoSample.rgb, mrSample.b);
    outNormalR = vec4(normalize(normal), mrSample.g);
    outEmissiveAO = vec4(emissiveSample.rgb, occlusionSample.r);

    outPosition = vec4(position, 1.0);
}