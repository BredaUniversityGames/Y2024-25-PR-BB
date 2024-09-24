#version 460

layout(set = 0, binding = 0) uniform sampler gBufferSampler;
layout(set = 0, binding = 1) uniform texture2D gBufferAlbedoM;    // RGB: Albedo,   A: Metallic
layout(set = 0, binding = 2) uniform texture2D gBufferNormalR;    // RGB: Normal,   A: Roughness
layout(set = 0, binding = 3) uniform texture2D gBufferEmissiveAO; // RGB: Emissive, A: AO
layout(set = 0, binding = 4) uniform texture2D gBufferPosition;   // RGB: Position, A: Unused
layout(set = 0, binding = 5) uniform samplerCube irradianceMap;
layout(set = 0, binding = 6) uniform samplerCube prefilterMap;
layout(set = 0, binding = 7) uniform sampler2D brdfLUT;
layout(set = 0, binding = 8) uniform sampler2DShadow shadowMap;

layout(set = 1, binding = 0) uniform CameraUBO
{
 mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 lightVP;
    mat4 depthBiasMVP;
    vec4 lightData;
    vec3 cameraPosition;
    float _padding;
} cameraUbo;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NoV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);

void main()
{
    vec4 albedoM = texture(sampler2D(gBufferAlbedoM, gBufferSampler), texCoords);
    vec4 normalR = texture(sampler2D(gBufferNormalR, gBufferSampler), texCoords);
    vec4 emissiveAO = texture(sampler2D(gBufferEmissiveAO, gBufferSampler), texCoords);
    vec3 position = texture(sampler2D(gBufferPosition, gBufferSampler), texCoords).xyz;

    vec3 albedo = albedoM.rgb;
    float metallic = albedoM.a;
    vec3 normal = normalR.xyz;
    float roughness = normalR.a;
    vec3 emissive = emissiveAO.rgb;
    float ao = emissiveAO.a;

    if (normal == vec3(0.0, 0.0, 0.0))
    discard;

    //vec3 lightDir = normalize(vec3(-0.5, 0.3, -0.3));
    vec3 lightDir = cameraUbo.lightData.xyz;
    vec3 Lo = vec3(0.0);

    vec3 N = normalize(normal);
    vec3 V = normalize(cameraUbo.cameraPosition - position);

    vec3 L = lightDir;
    vec3 H = normalize(V + L);
    vec3 radiance = vec3(244.0f, 183.0f, 64.0f) / 255.0f * 4.0; // Light color.
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    {
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;

        kD *= 1.0 - metallic;

        float NoL = max(dot(N, L), 0.0);

        Lo += (kD * albedo / PI + specular) * radiance * NoL;
    }

    vec3 R = reflect(V, N);
    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, -N).rgb;
    vec3 diffuse = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuse + specular) * ao;

    vec4 ShadowCoord = cameraUbo.depthBiasMVP * vec4(position, 1.0);
    vec4 TestCoord = cameraUbo.lightVP * vec4(position, 1.0);

    float cosTheta = clamp(dot(N, lightDir),0.0,1.0);
    float bias = max(0.005 * (1.0 - cosTheta), 0.0001);

    bias = clamp(bias, 0,0.005);

    const float offset = 1.0 / (textureSize(shadowMap,0).x*1.6); // Assuming a 4096x4096 shadow map

    float visibility = 1.0;
    float shadow = 0.0;
    float depthFactor = TestCoord.z - bias;
    shadow += texture(shadowMap, vec3(ShadowCoord.xy + vec2(-offset, -offset), depthFactor)).r;
    shadow += texture(shadowMap, vec3(ShadowCoord.xy + vec2(-offset,  offset), depthFactor)).r;
    shadow += texture(shadowMap, vec3(ShadowCoord.xy + vec2( offset, -offset), depthFactor)).r;
    shadow += texture(shadowMap, vec3(ShadowCoord.xy + vec2( offset,  offset), depthFactor)).r;
    shadow *= 0.25; // Average the samples

    visibility = clamp(shadow, 0.2, 1.0);


    outColor = vec4((Lo + ambient + emissive) * visibility, 1.0);
    //outColor = vec4((visibility.r),0.0,0.0,1.0);

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
