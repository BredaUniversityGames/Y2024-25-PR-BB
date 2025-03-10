#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"
#include "settings.glsl"
#include "octahedron.glsl"
#include "clusters.glsl"

layout (push_constant) uniform PushConstants
{
    uint albedoRMIndex;
    uint normalIndex;
    uint ssaoIndex;
    uint depthIndex;
    vec2 screenSize;
    vec2 padding;
    ivec3 clusterDimensions;
    float shadowMapSize;
    float ambientStrength;
    float ambientShadowStrength;
} pushConstants;

layout (set = 1, binding = 0) uniform CameraUBO
{
    Camera camera;
};
layout (set = 2, binding = 0) uniform SceneUBO
{
    Scene scene;
};

layout (set = 3, binding = 0) buffer PointLightSSBO
{
    PointLightArray pointLights;
};

layout (set = 4, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};

layout (set = 5, binding = 0) readonly buffer AtomicCount { uint count; };
layout (set = 5, binding = 1) readonly buffer LightCells { LightCell lightCells[]; };
layout (set = 5, binding = 2) readonly buffer LightIndices { uint lightIndices[]; };

layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outBrightness;

const float PI = 3.14159265359;


float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NoV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
vec3 CalculateBRDF(vec3 normal, vec3 view, vec3 lightDir, vec3 albedo, vec3 F0, float metallic, float roughness, vec3 lightColor);
float CalculateAttenuation(vec3 lightPos, vec3 position, float range);
float CalculateShadowBias(float cosTheta, float baseBias);
vec3 CalculateDiffuseIBL(vec3 normal, vec3 albedo, uint irradianceIndex);
vec3 CalculateSpecularIBL(vec3 normal, vec3 viewDir, float roughness, vec3 F, uint prefilterIndex, uint brdfLUTIndex);
void DirectionalShadowMap(vec3 position, float bias, inout float shadow);

vec3 applyFog(in vec3 color, in float distanceToPoint, in vec3 cameraPosition, in vec3 directionToCamera, in vec3 lightPosition);

// stackoverflow.com/questions/51108596/linearize-depth
float LinearDepth(float z, float near, float far)
{
    return near * far / (far + z * (near - far));
}

void main()
{
    vec4 albedoRMSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.albedoRMIndex)], texCoords);
    vec4 normalSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.normalIndex)], texCoords);
    float depthSample = texture(bindless_depth_textures[nonuniformEXT(pushConstants.depthIndex)], texCoords).r;
    float ambientOcclusion = texture(bindless_color_textures[nonuniformEXT(pushConstants.ssaoIndex)], texCoords).r;

    float linearDepthCulling = LinearDepth(1.0 - depthSample, camera.zNear, camera.zFar);

    float zFloat = pushConstants.clusterDimensions.z;
    float log2FarDivNear = log2(camera.zFar / camera.zNear);
    float log2Near = log2(camera.zNear);

    float sliceScaling = zFloat / log2FarDivNear;
    float sliceBias = -(zFloat * log2Near / log2FarDivNear);
    uint zIndex = uint(max(log2(linearDepthCulling) * sliceScaling + sliceBias, 0.0));
    vec2 tileSize =
    vec2(pushConstants.screenSize.x / float(16),
    pushConstants.screenSize.y / float(9));

    uvec3 cluster = uvec3(
    gl_FragCoord.x / tileSize.x,
    gl_FragCoord.y / tileSize.y,
    zIndex);


    uint clusterIndex =
    cluster.x +
    cluster.y * pushConstants.clusterDimensions.x +
    cluster.z * pushConstants.clusterDimensions.x * pushConstants.clusterDimensions.y;

    uint lightCount = lightCells[clusterIndex].count;
    uint lightIndexOffset = lightCells[clusterIndex].offset;

    vec3 albedo = albedoRMSample.rgb;
    float roughness;
    float metallic;
    DecodeRM(albedoRMSample.a, roughness, metallic);
    vec3 normal = OctDecode(normalSample.rg);
    vec3 position = ReconstructWorldPosition(depthSample, texCoords, camera.inverseVP);

    if (normal == vec3(0.0))
    discard;

    vec3 Lo = vec3(0.0);
    vec3 N = normalize(normal);
    vec3 V = normalize(camera.cameraPosition - position);
    vec3 R = reflect(V, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    //kD *= 1.0 - metallic;

    // Directional Light Calculations
    vec3 lightDir = scene.directionalLight.direction.xyz;
    float cosTheta = max(dot(N, lightDir), 0.0);
    float bias = CalculateShadowBias(cosTheta, scene.directionalLight.direction.w);
    Lo += CalculateBRDF(N, V, lightDir, albedo, F0, metallic, roughness, scene.directionalLight.color.rgb);

    vec3 pointLightLo = vec3(0.0);
    for (int i = 0; i < lightCount; i++)
    {
        uint lightIndex = lightIndices[i + lightIndexOffset];
        PointLight light = pointLights.lights[lightIndex];

        vec3 lightPos = light.position.xyz;
        vec3 L = normalize(lightPos - position);
        float attenuation = CalculateAttenuation(lightPos, position, light.range);
        vec3 lightColor = light.color.rgb * attenuation * light.intensity;

        pointLightLo += CalculateBRDF(N, V, L, albedo, F0, metallic, roughness, lightColor);
    }

    // IBL Contributions
    vec3 diffuseIBL = CalculateDiffuseIBL(N, albedo, scene.irradianceIndex);
    vec3 ambient = (kD * diffuseIBL) * ambientOcclusion;

    float shadow = 0.0;
    DirectionalShadowMap(position, bias, shadow);

    float ambientShadow = (1.0 - (1.0 - shadow) * pushConstants.ambientShadowStrength);

    vec3 litColor = vec3((Lo * shadow) + pointLightLo + (ambient * pushConstants.ambientStrength) * ambientShadow);

    float linearDepth = distance(position, camera.cameraPosition);
    outColor = vec4(applyFog(litColor, linearDepth, camera.cameraPosition, normalize(position - camera.cameraPosition), scene.directionalLight.direction.xyz), 1.0);

    // We store brightness for bloom later on
    float brightnessStrength = dot(outColor.rgb, bloomSettings.colorWeights);
    vec3 brightnessColor = outColor.rgb * (brightnessStrength * bloomSettings.gradientStrength);
    brightnessColor = min(brightnessColor, bloomSettings.maxBrightnessExtraction);
    outBrightness = vec4(brightnessColor, 1.0);
}

vec3 applyFog(in vec3 color, in float distanceToPoint, in vec3 cameraPosition, in vec3 directionToCamera, in vec3 lightPosition)
{
    float a = scene.fogHeight;
    float b = scene.fogDensity;
    float fogAmount = (a / b) * exp(-cameraPosition.y * b) * (1.0 - exp(-distanceToPoint * directionToCamera.y * b)) / directionToCamera.y;
    float sunAmount = max(dot(directionToCamera, lightPosition), 0.0);
    vec3 fogColor = mix(scene.fogColor,
                        scene.directionalLight.color.rgb,
                        pow(sunAmount, 8.0));
    return mix(color, fogColor, clamp(fogAmount, 0.0, 0.5));
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH = max(dot(N, H), 0.0);
    float NoH2 = NoH * NoH;

    float nom = a2;
    float denom = (NoH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NoV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float nom = NoV;
    float denom = NoV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NoV, roughness);
    float ggx2 = GeometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float CalculateAttenuation(vec3 lightPos, vec3 position, float range) {
    float distance = length(lightPos - position);
    return max(1.0 - (distance / range), 0.0) / (distance * distance);
}

float CalculateShadowBias(float cosTheta, float baseBias) {
    float bias = max(baseBias * (1.0 - cosTheta), baseBias);
    return clamp(bias, 0.0, baseBias);
}

vec3 CalculateDiffuseIBL(vec3 normal, vec3 albedo, uint irradianceIndex) {
    vec3 irradiance = texture(bindless_cubemap_textures[nonuniformEXT(irradianceIndex)], -normal).rgb;
    return irradiance * albedo;
}

vec3 CalculateSpecularIBL(vec3 normal, vec3 viewDir, float roughness, vec3 F, uint prefilterIndex, uint brdfLUTIndex) {
    const float MAX_REFLECTION_LOD = 5.0;
    vec3 R = reflect(viewDir, normal);
    vec3 prefilteredColor = textureLod(bindless_cubemap_textures[nonuniformEXT(prefilterIndex)], R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(bindless_color_textures[nonuniformEXT(brdfLUTIndex)], vec2(clamp(dot(normal, viewDir), 0.0, 0.99), roughness)).rg;
    return prefilteredColor * (F * envBRDF.x + envBRDF.y);
}

void DirectionalShadowMap(vec3 position, float bias, inout float shadow)
{
    const vec4 shadowCoord = scene.directionalLight.depthBiasMVP * vec4(position, 1.0);
    const vec4 testCoord = scene.directionalLight.lightVP * vec4(position, 1.0);

    float staticVisibility = 1.0;
    float dynamicVisibility = 1.0;
    const float depthFactor = testCoord.z - bias;

    for (int i = 0;i < 4; i++) {
        const int index = int(16.0 * randomIndex(floor(position.xyz * scene.directionalLight.poissonWorldOffset), i)) % 16;
        staticVisibility -= 0.25 * (1.0 - texture(bindless_shadowmap_textures[nonuniformEXT (scene.staticShadowMapIndex)], vec3(shadowCoord.xy + poissonDisk[index] / scene.directionalLight.poissonConstant, depthFactor / testCoord.w)).r);
        dynamicVisibility -= 0.25 * (1.0 - texture(bindless_shadowmap_textures[nonuniformEXT (scene.dynamicShadowMapIndex)], vec3(shadowCoord.xy + poissonDisk[index] / scene.directionalLight.poissonConstant, depthFactor / testCoord.w)).r);
    }
    shadow = min(staticVisibility, dynamicVisibility);
}


vec3 CalculateBRDF(vec3 normal, vec3 view, vec3 lightDir, vec3 albedo, vec3 F0, float metallic, float roughness, vec3 lightColor)
{
    vec3 H = normalize(view + lightDir);
    vec3 F = FresnelSchlick(max(dot(H, view), 0.0), F0);
    float NDF = DistributionGGX(normal, H, roughness);
    float G = GeometrySmith(normal, view, lightDir, roughness);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, view), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NoL = max(dot(normal, lightDir), 0.0);

    return (kD * albedo / PI + specular) * lightColor * NoL;
}

