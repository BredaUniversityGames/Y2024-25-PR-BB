#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"
#include "tonemapping.glsl"
#include "scene.glsl"
#include "octahedron.glsl"
#include "hashes.glsl"

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
    vec4 playerTrailPositions[32];
};

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

// --- Custom Uniforms for Hole Parameters ---
// These uniforms allow external control over the hole's properties.
// In a typical application, these would be set from the CPU (e.g., JavaScript in WebGL).
#define VOLUMETRIC_HEIGHT_OFFSET 10.0
float iHoleRadius = 0.1; // Radius of the hole
float iHoleFeather = 0.1; // Smoothness of the hole edges (blends from iHoleRadius to iHoleRadius + iHoleFeather)
// ---- Constants ----
#define MAX_STEPS 128
#define MAX_DIST 64.0

// ---- Noise / Hash as in your code ----
/**float hash(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453123);
}
float noise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = hash(i + vec3(0, 0, 0));
    float n100 = hash(i + vec3(1, 0, 0));
    float n010 = hash(i + vec3(0, 1, 0));
    float n110 = hash(i + vec3(1, 1, 0));
    float n001 = hash(i + vec3(0, 0, 1));
    float n101 = hash(i + vec3(1, 0, 1));
    float n011 = hash(i + vec3(0, 1, 1));
    float n111 = hash(i + vec3(1, 1, 1));
    float res = mix(
        mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
        mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y),
        f.z
    );
    return res;
}
float fractal_noise(vec3 p) {
    float f = 0.0;
    p = p - vec3(1.0, 1.0, 0.0) * pc.time * 0.1;
    p = p * 3.0;
    f += 0.50000 * noise(p); p = 2.0 * p;
    f += 0.25000 * noise(p); p = 2.0 * p;
    f += 0.12500 * noise(p); p = 2.0 * p;
    f += 0.06250 * noise(p); p = 2.0 * p;
    f += 0.03125 * noise(p);
    return f;
}*/

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
        float holeInfluence = smoothstep(decay + iHoleFeather, decay, dpr);
        den *= (1.0 - holeInfluence);
        // Hole (as before)
    }

    // Now apply player trail
    for (int i = 0; i < 32; i++) {
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
        float holeInfluence = smoothstep(1.0 + 0.4, 1.0, dpr);
        den *= (1.0 - holeInfluence);
    }

    // same thing for player
    vec3 playerPos = camera.cameraPosition;
    playerPos.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the player position to match the hole height
    float dpr = distance(playerPos, pos);
    float holeInfluence = smoothstep(1.0 + iHoleFeather, 1.0, dpr);
    den *= (1.0 - holeInfluence);

    // Add decay based on distance to the volume center

/**
    float distanceToVolume = distance(pos, vec3(-5.0, 9.595 - VOLUMETRIC_HEIGHT_OFFSET, 62.0));
    float decayFactor = smoothstep(0.0, 1.0, distanceToVolume * 0.05);
    den *= (1.0 - decayFactor);
*/

    den = clamp(den, 0.0, 0.85);
    return den;
}
/**vec3 color(float den, float y)
{
    vec3 result = mix(vec3(1.0, 0.9, 0.8 + sin(pc.time) * 0.1),
                      vec3(0.5, 0.15, 0.1 + sin(pc.time) * 0.1), den * den);
    // Apply a vertical (y) gradient, can be subtle for infinite volume
    vec3 colBot = 3.0 * vec3(1.0, 0.9, 0.5);
    vec3 colTop = 2.0 * vec3(0.5, 0.55, 0.55);
    result *= mix(colBot, colTop, smoothstep(-3.0, 3.0, y));
    result *= vec3(0.1, 0.05, 0.05);
    return result;
}*/

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


// ---- Camera ----
mat3 setCamera(vec3 ro, vec3 ta, float cr) {
    vec3 cw = normalize(ta - ro);
    vec3 cp = vec3(sin(cr), cos(cr), 0.0);
    vec3 cu = normalize(cross(cw, cp));
    vec3 cv = normalize(cross(cu, cw));
    return mat3(cu, cv, cw);
}
float BeerLambert(float absorptionCoefficient, float distanceTraveled)
{
    return exp(-absorptionCoefficient * distanceTraveled);
}

// ---- Raymarching through infinite volume ----
vec4 raymarching(vec3 ro, vec3 rd, float tmin, float tmax, vec3 sceneDepthPosition)
{
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




        lightTransmittance *= exp(-den * 0.000001 * stepSize);
        float lightFactor = max(0.001, dot(rd, getSunDirection()));

        // Combine ambient and directional light
        vec3 baseCloudColor = color(den, pos.y);



        vec3 litAmount = vec3(1.0) * (lightFactor * vec3(1.0, 0.8, 0.6)); // Adjusted for ambient light and directional light
        litAmount = clamp(litAmount, 0.0, 1.0); // Ensure it stays within valid range

        vec3 litCloudColor = baseCloudColor * litAmount * scene.directionalLight.color.rgb;

        vec4 col = vec4(baseCloudColor, den);
        col.rgb *= col.a; // Premultiply alpha
        sum = sum + col * (1.0 - sum.a);




        //t += 0.07 + 0.01 * float(i); // uniform or progressive steps



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


    ivec2 pixelCoords = ivec2(texCoords * vec2(texSize));
    const vec3 earlyRay = rayDirection(camera.fov, texSize, vec2(pixelCoords));
    const vec3 rayDirection = normalize(transpose(mat3(camera.view)) * earlyRay);

    vec2 p = vec2(texCoords * vec2(texSize));

    // Rotate the camera position around the origin
    vec3 ro = camera.cameraPosition; // Initial camera position
    ro.y -= VOLUMETRIC_HEIGHT_OFFSET;


    // Compute the ray direction from camera to pixel

    //float linearizedSceneDepth = getLinearSceneDepth(depthSample, camera.zNear, camera.zFar);

    vec3 pixelWorldPos = ReconstructWorldPosition(depthSample, texCoords, camera.inverseVP);
    pixelWorldPos.y -= VOLUMETRIC_HEIGHT_OFFSET; // Offset the pixel world position to match the hole height

    float dynamicMaxDist = MAX_DIST;
    dynamicMaxDist += max(0.0, ro.y) * 1.1f;

    outColor = raymarching(ro, rayDirection, 0.0, dynamicMaxDist, pixelWorldPos);



}
