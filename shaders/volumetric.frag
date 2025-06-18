#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"
#include "tonemapping.glsl"
#include "scene.glsl"
#include "octahedron.glsl"
#include "hashes.glsl"
#include "clusters.glsl"

struct GunShot {
    vec4 origin;
    vec4 direction;
};

layout (push_constant) uniform PushConstants
{
    uint hdrTargetIndex;
    uint bloomTargetIndex;
    uint depthIndex;
    uint normalRIndex;

    uint screenWidth;
    uint screenHeight;
    float time;
    uint _padding1;

} pc;

layout (set = 1, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};
layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};
layout (set = 3, binding = 0) uniform SceneUBO
{
    Scene scene;
};

layout (set = 4, binding = 0) uniform FogTrailsUBO {
    GunShot gunShots[8];
    vec4 playerTrailPositions[24];
};

layout (set = 5, binding = 0) buffer PointLightSSBO
{
    PointLightArray pointLights;
};

layout (set = 6, binding = 0) readonly buffer AtomicCount { uint count; };
layout (set = 6, binding = 1) readonly buffer LightCells { LightCell lightCells[]; };
layout (set = 6, binding = 2) readonly buffer LightIndices { uint lightIndices[]; };

layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;


vec3 getSunDirection() {
    const vec3 lightDir = normalize(scene.directionalLight.direction.xyz); //sunDirection
    return lightDir;
}


float linearize_depth(float d, float zNear, float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

// adding volumetric fog? god please help me

// --- Custom variables ---
#define VOLUMETRIC_HEIGHT_OFFSET 10.0
#define  HOLE_FEATHER 0.1
// ---- Constants ----
#define MAX_STEPS 128
#define MAX_DIST 64.0

// ---- Noise / Hash as in your code ----

mat3 m = mat3(0.00, 1.60, 1.20, -1.60, 0.72, -0.96, -1.20, -0.96, 1.28);
// hash function
float hash(float n)
{
    return fract(cos(n) * 114514.1919);
}

// 3d noise function
float noise(in vec3 x)
{
    vec3 p = floor(x);
    vec3 f = smoothstep(0.0, 1.0, fract(x));

    float n = p.x + p.y * 10.0 + p.z * 100.0;

    return mix(
        mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
            mix(hash(n + 10.0), hash(n + 11.0), f.x), f.y),
        mix(mix(hash(n + 100.0), hash(n + 101.0), f.x),
            mix(hash(n + 110.0), hash(n + 111.0), f.x), f.y), f.z);
}

// Fractional Brownian motion
float fbm(vec3 p)
{
    p = p - vec3(1.5, 0.2, 0.0) * pc.time * 0.18;

    float f = 0.5000 * noise(p);
    p = m * p;
    f += 0.2500 * noise(p);
    p = m * p;
    f += 0.1666 * noise(p);
    p = m * p;
    f += 0.0834 * noise(p);
    return f;
}

// ---- Hole logic ----
float distPointToRay(vec3 p, vec3 rayOrigin, vec3 rayDir) {
    vec3 op = p - rayOrigin;
    float t = dot(op, rayDir);
    vec3 closestPoint = rayOrigin + t * rayDir;
    return length(p - closestPoint);
}

//https://iquilezles.org/articles/distfunctions/
float sdVerticalCapsule(vec3 p, float h, float r)
{
    p.y -= clamp(p.y, 0.0, h);
    return length(p) - r;
}

// ---- Infinite Density Field ----
float density(vec3 pos)
{



    // Just noise-based, optionally bias for "height" (e.g., less dense at y>2 or y<-2)
    float base = smoothstep(0.5, 1.0, fbm(vec3(pos.x * 0.1, pos.y * 0.2, pos.z * 0.1)));
    float den = base * 1.4 - 0.2 - smoothstep(2.0, 4.0, abs(pos.y));

    // Fade out when z > 105 (starts at 105, fully gone at 110 for example)
    float zFade = 1.0 - smoothstep(95.0, 100.0, pos.z);
    den *= zFade;

    den = clamp(den, 0.0, 1.0);

    for (int i = 0; i < 8; i++) {

        const GunShot pcGunShot = gunShots[i];
        const float decay = pcGunShot.origin.a;
        if (decay < 0.001)
        {
            continue; // Skip if the gunshot is not active
        }
        vec3 origin = pcGunShot.origin.xyz;
        origin.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the origin to match the hole height
        vec3 rayDir = normalize(pcGunShot.direction.xyz);
        //rayDir.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the ray direction to match the hole height

        float dpr = distPointToRay(pos, origin, rayDir);
        float holeInfluence = smoothstep(decay + HOLE_FEATHER, decay, dpr);
        den *= (1.0 - holeInfluence);
        // Hole (as before)
    }

    // Now apply player trail
    for (int i = 0; i < 24; i++) {
        const vec4 playerTrail = playerTrailPositions[i];
        const float decay = playerTrail.a;
        if (decay < 0.001)
        {
            continue; // Skip if the player trail is not active
        }
        vec3 origin = playerTrail.xyz;
        origin.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the origin to match the hole height
        origin.y -= 1.6; // Additional offset to lower the hole height
        float dpr = sdVerticalCapsule(pos - origin, 2.1, 0.08);
        float holeInfluence = smoothstep(decay + 0.4, decay, dpr);
        den *= (1.0 - holeInfluence);
    }

    // same thing for player
    vec3 playerPos = camera.cameraPosition;
    playerPos.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the player position to match the hole height
    float dpr = distance(playerPos, pos);
    float holeInfluence = smoothstep(1.6 + HOLE_FEATHER, 1.6, dpr);
    den *= (1.0 - holeInfluence);

    den = clamp(den, 0.0, 0.85);
    return den;
}

vec3 color(float den, float y)
{
    // add animation
    vec3 result = mix(vec3(1.0, 0.9, 0.8 + sin(pc.time) * 0.1),
                      vec3(0.5, 0.15, 0.1 + sin(pc.time) * 0.1), den * den);


    vec3 colBot = 3.0 * vec3(1.0, 0.9, 0.5);
    vec3 colTop = 2.0 * vec3(0.5, 0.55, 0.55);
    result *= mix(colBot, colTop, smoothstep(-3.0, 3.0, y));
    result *= vec3(0.1, 0.05, 0.05);
    return result;
}


float CalculateAttenuation(vec3 lightPos, vec3 position, float range) {
    float distance = length(lightPos - position);
    return max(1.0 - (distance / range), 0.0) / (distance * distance);
}

// ---- Raymarching through infinite volume ----
vec4 raymarching(vec3 ro, vec3 rd, float tmin, float tmax, vec3 sceneDepthPosition)
{
    // Clusters fetching
    const ivec2 texSize = textureSize(bindless_color_textures[nonuniformEXT(pc.depthIndex)], 0);

    const float zFloat = 24;
    const float log2FarDivNear = log2(camera.zFar / camera.zNear);
    const float log2Near = log2(camera.zNear);

    const float sliceScaling = zFloat / log2FarDivNear;
    const float sliceBias = -(zFloat * log2Near / log2FarDivNear);
    const vec2 tileSize = vec2((texSize.x / 6.0) / float(16), (texSize.y / 6.0) / float(9));

    const uvec3 clusterNoZ = uvec3(gl_FragCoord.x / tileSize.x, gl_FragCoord.y / tileSize.y, 0);
    //



    vec4 sum = vec4(0.0);
    float t = tmin;

    // Calculate the distance from the ray origin to the opaque object's position
    // This value represents how far we need to march along 'rd' to hit the opaque object.
    float distToOpaqueObject = distance(ro, sceneDepthPosition);

    float lightTransmittance = 1.0;



    for (int i = 0; i < MAX_STEPS; i++)
    {
        // Stop if we've accumulated enough opacity or gone too far
        if (sum.a > 0.99 || t > tmax) break;

        // Early out if the current raymarching distance 't' has exceeded
        // the distance to the opaque object.
        // Also check sum.a to ensure we don't prematurely clip a dense cloud
        // that starts before the opaque object but extends slightly past its depth.

        if (t > distToOpaqueObject) {
            break;
        }

        vec3 pos = ro + rd * t;

        if (pos.y > 25.0 || pos.y < -3.0) {
            break; // outside volume
        }

        float den = density(pos);
        float stepSize = 0.03; // Minimum step size
        if (den < 0.005) { // If very low density, step faster
                           stepSize = 0.3;
        } else if (den > 0.8) { // If very high density, step slower for detail
                                stepSize = 0.03;
        } else {
            stepSize = mix(0.03, 0.3, den); // Interpolate
        }

        // Combine ambient and directional light
        vec3 baseCloudColor = color(den, pos.y);

        vec3 scatteredLight = vec3(0.0);

        const uint zIndex = uint(max(log2(t) * sliceScaling + sliceBias, 0.0));
        const uint clusterIndex = clusterNoZ.x + clusterNoZ.y * 16 + zIndex * 16 * 9;
        const uint lightCount = lightCells[clusterIndex].count;
        const uint lightIndexOffset = lightCells[clusterIndex].offset;
        for (int i = 0; i < lightCount; i++)
        {
            uint lightIndex = lightIndices[i + lightIndexOffset];
            PointLight light = pointLights.lights[lightIndex];

            vec3 lightPos = light.position.xyz;
            lightPos.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the light position to match the hole height
            float attenuation = CalculateAttenuation(lightPos, pos, light.range);

            float phase = 1.0; // Or use Henyey-Greenstein, etc.
            // Optionally, add a shadow test here

            scatteredLight += light.color.rgb * attenuation * light.intensity * phase;

        }

        scatteredLight *= den * stepSize;
        vec3 fogColor = (baseCloudColor + scatteredLight);

        vec4 col = vec4(fogColor, den); // Step alpha = extinction * step size
        col.rgb *= col.a; // Premultiply alpha


        sum = sum + col * (1.0 - sum.a);

        t += stepSize + 0.01 * float(i);
    }
    sum = clamp(sum, 0.0, 1.0);
    return sum;
}


//



vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord)
{
    vec2 xy = fragCoord - size / 2.0;
    xy.y = -xy.y; // Invert Y coordinate to match OpenGL's coordinate system
    float z = (size.y / 2.0) / tan((fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}

float getLinearSceneDepth(float rawDepthValue, float near, float far) {
    // This formula inverts the common perspective projection depth buffer mapping
    // where 1.0 is near and 0.0 is far.
    // It correctly converts the non-linear, inverted depth to a linear world-space distance.
    return 1.0 / (rawDepthValue * (1.0 / near - 1.0 / far) + 1.0 / far);
}



void main()
{

    const ivec2 texSize = textureSize(bindless_color_textures[nonuniformEXT(pc.depthIndex)], 0);
    float depthSample = texture(bindless_depth_textures[nonuniformEXT (pc.depthIndex)], texCoords).r;

    const ivec2 pixelCoords = ivec2(texCoords * vec2(texSize));
    const vec3 earlyRay = rayDirection(camera.fov, texSize, vec2(pixelCoords));
    const vec3 rayDirection = normalize(transpose(mat3(camera.view)) * earlyRay);


    vec3 ro = camera.cameraPosition;
    ro.y -= VOLUMETRIC_HEIGHT_OFFSET;

    vec3 pixelWorldPos = ReconstructWorldPosition(depthSample, texCoords, camera.inverseVP);
    pixelWorldPos.y -= VOLUMETRIC_HEIGHT_OFFSET;

    float dynamicMaxDist = MAX_DIST;
    dynamicMaxDist += max(0.0, ro.y) * 1.1f;

    outColor = raymarching(ro, rayDirection, 0.0, dynamicMaxDist, pixelWorldPos);



}
