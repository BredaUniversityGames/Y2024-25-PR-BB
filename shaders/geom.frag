#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in mat3 TBN;

layout(location = 0) out vec4 outAlbedoM;    // RGB: Albedo,   A: Metallic
layout(location = 1) out vec4 outNormalR;    // RGB: Normal,   A: Roughness
layout(location = 2) out vec4 outEmissiveAO; // RGB: Emissive, A: AO
layout(location = 3) out vec4 outPosition;   // RGB: Position, A: Unused

layout(set = 2, binding = 0) uniform sampler imageSampler;
layout(set = 2, binding = 1) uniform texture2D albedoImage;
layout(set = 2, binding = 2) uniform texture2D mrImage;
layout(set = 2, binding = 3) uniform texture2D normalImage;
layout(set = 2, binding = 4) uniform texture2D occlusionImage;
layout(set = 2, binding = 5) uniform texture2D emissiveImage;
layout(set = 2, binding = 6) uniform MaterialInfoUBO
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
    float _padding1;
} materialInfoUBO;

void main()
{
    vec4 albedoSample = pow(materialInfoUBO.albedoFactor, vec4(2.2));
    vec4 mrSample = vec4(materialInfoUBO.metallicFactor, materialInfoUBO.metallicFactor, 1.0, 1.0);
    vec4 occlusionSample = vec4(materialInfoUBO.occlusionStrength);
    vec4 emissiveSample = pow(vec4(materialInfoUBO.emissiveFactor, 0.0), vec4(2.2));

    vec3 normal = normalIn;

    if(materialInfoUBO.useAlbedoMap)
    {
        albedoSample *= pow(texture(sampler2D(albedoImage, imageSampler), texCoord), vec4(2.2));
    }
    if(materialInfoUBO.useMRMap)
    {
        mrSample *= texture(sampler2D(mrImage, imageSampler), texCoord);
    }
    if(materialInfoUBO.useNormalMap)
    {
        vec4 normalSample = texture(sampler2D(normalImage, imageSampler), texCoord) * materialInfoUBO.normalScale;
        normal = normalSample.xyz * 2.0 - 1.0;
        normal = normalize(TBN * normal);
    }
    if(materialInfoUBO.useOcclusionMap)
    {
        occlusionSample *= texture(sampler2D(occlusionImage, imageSampler), texCoord);
    }
    if(materialInfoUBO.useEmissiveMap)
    {
        emissiveSample *= pow(texture(sampler2D(emissiveImage, imageSampler), texCoord), vec4(2.2));
    }

    outAlbedoM = vec4(albedoSample.rgb, mrSample.b);
    outNormalR = vec4(normalize(normal), mrSample.g);
    outEmissiveAO = vec4(emissiveSample.rgb, occlusionSample.r);

    outPosition = vec4(position, 1.0);
}