#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "scene.glsl"
#include "settings.glsl"
#include "octahedron.glsl"

layout (push_constant) uniform PushConstants
{
    uint albedoRMIndex;
    uint normalIndex;
    uint ssaoIndex;
    uint depthIndex;
    float shadowMapSize;
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

layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outBrightness;

const float PI = 3.14159265359;

const vec2 poissonDisk[16] = vec2[](
vec2(-0.94201624, -0.39906216),
vec2(0.94558609, -0.76890725),
vec2(-0.094184101, -0.92938870),
vec2(0.34495938, 0.29387760),
vec2(-0.91588581, 0.45771432),
vec2(-0.81544232, -0.87912464),
vec2(-0.38277543, 0.27676845),
vec2(0.97484398, 0.75648379),
vec2(0.44323325, -0.97511554),
vec2(0.53742981, -0.47373420),
vec2(-0.26496911, -0.41893023),
vec2(0.79197514, 0.19090188),
vec2(-0.24188840, 0.99706507),
vec2(-0.81409955, 0.91437590),
vec2(0.19984126, 0.78641367),
vec2(0.14383161, -0.14100790)
);


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
void DirectionalShadowMap(vec3 position, float bias, vec3 worldPos, inout float shadow);

vec3 applyFog(in vec3 color, in float distanceToPoint, in vec3 cameraPosition, in vec3 directionToCamera, in vec3 lightPosition);

void main()
{
    vec4 albedoRMSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.albedoRMIndex)], texCoords);
    vec4 normalSample = texture(bindless_color_textures[nonuniformEXT(pushConstants.normalIndex)], texCoords);
    float depthSample = texture(bindless_depth_textures[nonuniformEXT(pushConstants.depthIndex)], texCoords).r;
    float ambientOcclusion = texture(bindless_color_textures[nonuniformEXT(pushConstants.ssaoIndex)], texCoords).r;

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
    kD *= 1.0 - metallic;

    // Directional Light Calculations
    vec3 lightDir = scene.directionalLight.direction.xyz;
    float cosTheta = max(dot(N, lightDir), 0.0);
    float bias = CalculateShadowBias(cosTheta, scene.directionalLight.direction.w);
    Lo += CalculateBRDF(N, V, lightDir, albedo, F0, metallic, roughness, scene.directionalLight.color.rgb);

    // Point Light Calculations
    for (int i = 0; i < pointLights.count; i++) {
        PointLight light = pointLights.lights[i];
        vec3 L = normalize(light.position - position);
        float attenuation = CalculateAttenuation(light.position, position, light.range);
        vec3 lightColor = light.color * attenuation;

        Lo += CalculateBRDF(N, V, L, albedo, F0, metallic, roughness, lightColor);
    }

    // IBL Contributions
    vec3 diffuseIBL = CalculateDiffuseIBL(N, albedo, scene.irradianceIndex);
    vec3 ambient = (kD * diffuseIBL) * ambientOcclusion;

    float shadow = 0.0;
    DirectionalShadowMap(position, bias, position, shadow);

    float ambientShadow = (1.0 - (1.0 - shadow) * 0.5);

    vec3 litColor = vec3((Lo * shadow) + ambient * ambientShadow);

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
    return clamp(1.0 - (distance / range), 0.0, 1.0);
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


float randomIndex(vec3 seed, int i) {
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

void DirectionalShadowMap(vec3 position, float bias, vec3 worldPos, inout float shadow)
{
    vec4 shadowCoord = scene.directionalLight.depthBiasMVP * vec4(position, 1.0);
    vec4 testCoord = scene.directionalLight.lightVP * vec4(position, 1.0);
    const float offset = 1.0 / (pushConstants.shadowMapSize * 1.6);

    float staticVisibility = 1.0;
    float dynamicVisibility = 1.0;
    float depthFactor = testCoord.z - bias;

    for (int i = 0;i < 4; i++) {
        const int index = int(16.0 * randomIndex(floor(worldPos.xyz * scene.directionalLight.poissonWorldOffset), i)) % 16;
        staticVisibility -= 0.25 * (1.0 - texture(bindless_shadowmap_textures[nonuniformEXT (scene.staticShadowMapIndex)], vec3(shadowCoord.xy + poissonDisk[index] / scene.directionalLight.poissonConstant, (testCoord.z - bias) / testCoord.w)).r);
        dynamicVisibility -= 0.25 * (1.0 - texture(bindless_shadowmap_textures[nonuniformEXT (scene.dynamicShadowMapIndex)], vec3(shadowCoord.xy + poissonDisk[index] / scene.directionalLight.poissonConstant, (testCoord.z - bias) / testCoord.w)).r);
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

