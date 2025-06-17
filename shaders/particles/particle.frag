#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "../bindless.glsl"
#include "../scene.glsl"
#include "../settings.glsl"
#include "particle_vars.glsl"

layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (set = 3, binding = 0) uniform SceneUBO
{
    Scene scene;
};

layout (set = 4, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normalIn;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec2 texCoord2;
layout (location = 4) flat in uint materialIndex;
layout (location = 5) flat in uint flags;
layout (location = 6) in vec3 colorIn;
layout (location = 7) in float frameBlend;
layout (location = 8) in float alpha;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outBrightness;

/// Gpt o1 random hash function
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
///

float CalculateShadowBias(float cosTheta, float baseBias);
void DirectionalShadowMap(vec3 position, float bias, inout float shadow);

void main()
{
    vec4 color = pow(texture(bindless_color_textures[nonuniformEXT(materialIndex)], texCoord), vec4(2.2));

    if ((flags & FRAMEBLEND) == FRAMEBLEND)
    {
        vec4 color2 = pow(texture(bindless_color_textures[nonuniformEXT(materialIndex)], texCoord2), vec4(2.2));

        color = mix(color, color2, frameBlend);
    }

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

    if (color.a < 0.2f)
    {
        discard;
    }

    float shadow = 1.0;
    if ((flags & NOSHADOW) != NOSHADOW)
    {
        shadow = 0.0;
        vec3 N = normalize(normalIn);
        vec3 lightDir = scene.directionalLight.direction.xyz;
        float cosTheta = max(dot(N, lightDir), 0.0);
        float bias = CalculateShadowBias(cosTheta, scene.directionalLight.direction.w);
        DirectionalShadowMap(position, bias, shadow);
        // TODO: find solution to temporary avoiding of hard shadows
        shadow += 0.1;
    }

    color *= vec4(colorIn, 0.0);

    if ((flags & UNLIT) != UNLIT)
    {
        color *= vec4(scene.directionalLight.color.xyz, 0.0);
    }

    vec3 litColor = shadow * color.xyz;

    const float fogDensity = 0.0025;
    const vec3 fogColor = vec3(0.6, 0.7, 0.9);

    float linearDepth = distance(position, camera.cameraPosition);
    float fogFactor = exp(-fogDensity * linearDepth);

    outColor = vec4(mix(fogColor, litColor, fogFactor), 1.0);

    // store brightness for bloom
    float brightnessStrength = dot(outColor.rgb, bloomSettings.colorWeights);
    vec3 brightnessColor = outColor.rgb * (brightnessStrength * bloomSettings.gradientStrength);
    brightnessColor = min(brightnessColor, bloomSettings.maxBrightnessExtraction);
    outBrightness = vec4(brightnessColor, 1.0);
}

float CalculateShadowBias(float cosTheta, float baseBias) {
    float bias = max(baseBias * (1.0 - cosTheta), baseBias);
    return clamp(bias, 0.0, baseBias);
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
