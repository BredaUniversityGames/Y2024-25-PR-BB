#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"
#include "tonemapping.glsl"
#include "scene.glsl"
#include "octahedron.glsl"
#include "hashes.glsl"


#define ENABLE_VIGNETTE        (1 << 0)
#define ENABLE_LENS_DISTORTION (1 << 1)
#define ENABLE_TONE_ADJUSTMENTS (1 << 2)
#define ENABLE_PIXELIZATION (1 << 3)
#define ENABLE_PALETTE (1 << 4)

layout (push_constant) uniform PushConstants
{
    uint hdrTargetIndex;
    uint bloomTargetIndex;
    uint depthIndex;
    uint enableFlags;

    uint normalIndex;
    uint tonemappingFunction;
    uint volumetricIndex;
    float exposure;

    float vignetteIntensity;
    float lensDistortionIntensity;
    float lensDistortionCubicIntensity;
    float screenScale;

    float brightness;
    float contrast;
    float saturation;
    float vibrance;

    float hue;
    float minPixelSize;
    float maxPixelSize;
    float pixelizationLevels;

    float pixelizationDepthBias;
    uint screenWidth;
    uint screenHeight;
    float ditherAmount;

    float paletteAmount;
    float time;
    float cloudsSpeed;
    uint paletteSize;

    vec4 skyColor;
    vec4 sunColor;
    vec4 cloudsColor;
    vec4 voidColor;

    vec4 flashColor;
    vec4 waterColor;

    vec4 rayOrigin;

    vec4 rayDirection;


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

layout (set = 4, binding = 0) uniform ColorPaletteUBO {
    vec4 palette[64];
};

layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

vec3 Vignette(in vec3 color, in vec2 texCoords, in float intensity);
vec3 ScreenFlash(in vec3 color, in vec3 flashColor, in vec2 texCoords, in float intensity);
vec2 LensDistortionUV(vec2 uv, float k, float kcube);
void BrightnessAdjust(inout vec3 color, in float b);
void ContrastAdjust(inout vec3 color, in float c);
mat4 SaturationMatrix(float saturation);
int Modi(int x, int y);
int And(int a, int b);
vec3 Vibrance(vec3 inCol, float vibrance);
vec3 ShiftHue(in vec3 col, in float Shift);
vec2 ComputePixelatedUV(float depthSample, float levels, float minPixelSize, float maxPixelSize, vec2 texCoords, vec2 screenSize);
vec3 SaturateColor(vec3 color, float saturationFactor);
vec3 ComputeQuantizedColor(vec3 color, float ditherAmount, float blendFactor);
float noise(in vec2 uv);
float fbm(in vec2 uv);
vec3 Sky(in vec3 ro, in vec3 rd, in vec3 waterColor);


// 4x4 Bayer matrix with values 0..15
const float bayer[4][4] = float[4][4](
float[4](0.0, 8.0, 2.0, 10.0),
float[4](12.0, 4.0, 14.0, 6.0),
float[4](3.0, 11.0, 1.0, 9.0),
float[4](15.0, 7.0, 13.0, 5.0)
);


float linearize_depth(float d, float zNear, float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

// ADDING WATER
// Based on: https://www.shadertoy.com/view/MdXyzX

// Use your mouse to move the camera around! Press the Left Mouse Button on the image to look around!

#define DRAG_MULT 0.38 // changes how much waves pull on the water
#define WATER_DEPTH 1.5 // how deep is the water
#define CAMERA_HEIGHT 2.8 // how high the camera should be
#define ITERATIONS_RAYMARCH 8 // waves iterations of raymarching
#define ITERATIONS_NORMAL 24 // waves iterations when calculating normals

// Calculates wave value and its derivative,
// for the wave direction, position in space, wave frequency and time
vec2 wavedx(vec2 position, vec2 direction, float frequency, float timeshift) {
    float x = dot(direction, position) * frequency + timeshift;
    float wave = exp(sin(x) - 1.0);
    float dx = wave * cos(x);
    return vec2(wave, -dx);
}

// Calculates waves by summing octaves of various waves with various parameters
float getwaves(vec2 position, int iterations) {
    float wavePhaseShift = length(position) * 0.1; // this is to avoid every octave having exactly the same phase everywhere
    float iter = 0.0; // this will help generating well distributed wave directions
    float frequency = 1.0; // frequency of the wave, this will change every iteration
    float timeMultiplier = 2.0; // time multiplier for the wave, this will change every iteration
    float weight = 1.0;// weight in final sum for the wave, this will change every iteration
    float sumOfValues = 0.0; // will store final sum of values
    float sumOfWeights = 0.0; // will store final sum of weights
    for (int i = 0; i < iterations; i++) {
        // generate some wave direction that looks kind of random
        vec2 p = vec2(sin(iter), cos(iter));

        // calculate wave data
        vec2 res = wavedx(position, p, frequency, pc.time * timeMultiplier + wavePhaseShift);

        // shift position around according to wave drag and derivative of the wave
        position += p * res.y * weight * DRAG_MULT;

        // add the results to sums
        sumOfValues += res.x * weight;
        sumOfWeights += weight;

        // modify next octave ;
        weight = mix(weight, 0.0, 0.2);
        frequency *= 1.18;
        timeMultiplier *= 1.07;

        // add some kind of random value to make next wave look random too
        iter += 1232.399963;
    }
    // calculate and return
    return sumOfValues / sumOfWeights;
}

// Raymarches the ray from top water layer boundary to low water layer boundary
float raymarchwater(vec3 camera, vec3 start, vec3 end, float depth) {
    vec3 pos = start;
    vec3 dir = normalize(end - start);
    for (int i = 0; i < 4; i++) {
        // the height is from 0 to -depth
        float height = getwaves(pos.xz, ITERATIONS_RAYMARCH) * depth - depth;
        // if the waves height almost nearly matches the ray height, assume its a hit and return the hit distance
        if (height + 0.01 > pos.y) {
            return distance(pos, camera);
        }
        // iterate forwards according to the height mismatch
        pos += dir * (pos.y - height);
    }
    // if hit was not registered, just assume hit the top layer,
    // this makes the raymarching faster and looks better at higher distances
    return distance(start, camera);
}

// Calculate normal at point by calculating the height at the pos and 2 additional points very close to pos
vec3 normal(vec2 pos, float e, float depth) {
    vec2 ex = vec2(e, 0);
    float H = getwaves(pos.xy, ITERATIONS_NORMAL) * depth;
    vec3 a = vec3(pos.x, H, pos.y);
    return normalize(
        cross(
            a - vec3(pos.x - e, getwaves(pos.xy - ex.xy, ITERATIONS_NORMAL) * depth, pos.y),
            a - vec3(pos.x, getwaves(pos.xy + ex.yx, ITERATIONS_NORMAL) * depth, pos.y + e)
        )
    );
}


// Ray-Plane intersection checker
float intersectPlane(vec3 origin, vec3 direction, vec3 point, vec3 normal) {
    return clamp(dot(point - origin, normal) / dot(direction, normal), -1.0, 9991999.0);
}

// Some very barebones but fast atmosphere approximation
vec3 extra_cheap_atmosphere(vec3 raydir, vec3 sundir) {

    vec3 waterColor = pc.waterColor.rgb * pc.waterColor.w;
    //sundir.y = max(sundir.y, -0.07);
    float special_trick = 1.0 / (raydir.y * 1.0 + 0.1);
    float special_trick2 = 1.0 / (sundir.y * 11.0 + 1.0);
    float raysundt = pow(abs(dot(sundir, raydir)), 2.0);
    float sundt = pow(max(0.0, dot(sundir, raydir)), 8.0);
    float mymie = sundt * special_trick * 0.2;
    vec3 suncolor = mix(vec3(1.0), max(vec3(0.0), vec3(1.0) - waterColor / 22.4), special_trick2);
    vec3 bluesky = waterColor / 22.4 * suncolor;
    vec3 bluesky2 = max(vec3(0.0), bluesky - waterColor * 0.002 * (special_trick + -6.0 * sundir.y * sundir.y));
    bluesky2 *= special_trick * (0.24 + raysundt * 0.24);
    return bluesky2 * (1.0 + 1.0 * pow(1.0 - raydir.y, 3.0));
}

// Calculate where the sun should be, it will be moving around the sky
vec3 getSunDirection() {
    const vec3 lightDir = normalize(scene.directionalLight.direction.xyz); //sunDirection
    return lightDir;
}

// Get atmosphere color for given direction
vec3 getAtmosphere(vec3 dir) {
    return extra_cheap_atmosphere(dir, getSunDirection()) * 0.5;
}

// Get sun color for given direction
float getSun(vec3 dir) {
    return pow(max(0.0, dot(dir, getSunDirection())), 720.0) * 210.0;
}

vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord)
{
    vec2 xy = fragCoord - size / 2.0;
    xy.y = -xy.y; // Invert y-axis for correct orientation
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
    vec2 newTexCoords = texCoords;
    const uint enableFlags = pc.enableFlags;
    const bool vignetteEnabled = bool(enableFlags & ENABLE_VIGNETTE);
    const bool lensDistortionEnabled = bool(enableFlags & ENABLE_LENS_DISTORTION);
    const bool toneAdjustmentsEnabled = bool(enableFlags & ENABLE_TONE_ADJUSTMENTS);
    const bool pixelizationEnabled = bool(enableFlags & ENABLE_PIXELIZATION);
    const bool paletteEnabled = bool(enableFlags & ENABLE_PALETTE);

    if (lensDistortionEnabled)
    {
        newTexCoords = LensDistortionUV(texCoords, pc.lensDistortionIntensity, pc.lensDistortionCubicIntensity);
        newTexCoords = (newTexCoords - 0.5) * pc.screenScale + 0.5;
    }
    // Prepare the circle parameters, cycling the circle size over time.
    const vec3 bloomColor = texture(bindless_color_textures[nonuniformEXT (pc.bloomTargetIndex)], newTexCoords).rgb;
    float depthSample = texture(bindless_depth_textures[nonuniformEXT (pc.depthIndex)], newTexCoords).r;

    vec3 hdrColor = vec3(0.0);
    const ivec2 texSize = textureSize(bindless_color_textures[nonuniformEXT(pc.depthIndex)], 0);
    if (pixelizationEnabled)
    {
        const vec2 uv = ComputePixelatedUV(depthSample, pc.pixelizationLevels, pc.minPixelSize, pc.maxPixelSize, newTexCoords, vec2(pc.screenWidth, pc.screenHeight));
        newTexCoords = uv;
        const ivec2 pixelCoords = ivec2(newTexCoords * vec2(texSize));
        hdrColor = texelFetch(bindless_color_textures[nonuniformEXT (pc.hdrTargetIndex)], pixelCoords, 0).rgb;
    } else
    {
        const ivec2 pixelCoords = ivec2(newTexCoords * vec2(texSize));
        hdrColor = texelFetch(bindless_color_textures[nonuniformEXT (pc.hdrTargetIndex)], pixelCoords, 0).rgb;
    }

/**    if (paletteEnabled)
    {
        hdrColor = ComputeQuantizedColor(hdrColor, pc.ditherAmount, pc.paletteAmount);
    }*/

    vec3 bloom = bloomColor * bloomSettings.strength;
    hdrColor += bloom;
    vec3 color = vec3(1.0) - exp(-hdrColor * pc.exposure);

    //sample the depth again, maybe we now need to use pixelization
    ivec2 pixelCoords = ivec2(newTexCoords * vec2(texSize));
    float pixelatedDepthSample = texelFetch(bindless_color_textures[nonuniformEXT (pc.depthIndex)], pixelCoords, 0).r;

    const vec3 earlyRay = rayDirection(camera.fov, texSize, vec2(pixelCoords));
    const vec3 rayDirection = normalize(transpose(mat3(camera.view)) * earlyRay);

    ivec2 halfPixelCoords = ivec2(newTexCoords * (vec2(texSize) / 6.0));
    vec4 volumetricSample = texelFetch(bindless_color_textures[nonuniformEXT (pc.volumetricIndex)], halfPixelCoords, 0);

    if (pixelatedDepthSample <= 0.0f)
    {
        vec3 waterColor = pc.voidColor.rgb;
        {//water
         ivec2 pixelCoords = ivec2(newTexCoords * vec2(texSize));

         if (rayDirection.y < 0)
         {



             // now ray.y must be negative, water must be hit
             // define water planes
             vec3 waterPlaneHigh = vec3(0.0, 0.0, 0.0);
             vec3 waterPlaneLow = vec3(0.0, -WATER_DEPTH, 0.0);

             // define ray origin, moving around
             vec3 origin = vec3((pc.time * 0.2) + camera.cameraPosition.x * 0.275, CAMERA_HEIGHT + camera.cameraPosition.y * 0.275, 1 + camera.cameraPosition.z * 0.275);

             // calculate intersections and reconstruct positions
             float highPlaneHit = intersectPlane(origin, rayDirection, waterPlaneHigh, vec3(0.0, 1.0, 0.0));
             float lowPlaneHit = intersectPlane(origin, rayDirection, waterPlaneLow, vec3(0.0, 1.0, 0.0));
             vec3 highHitPos = origin + rayDirection * highPlaneHit;
             vec3 lowHitPos = origin + rayDirection * lowPlaneHit;

             // raymatch water and reconstruct the hit pos
             float dist = raymarchwater(origin, highHitPos, lowHitPos, WATER_DEPTH);
             vec3 waterHitPos = origin + rayDirection * dist;

             // calculate normal at the hit position
             vec3 N = normal(waterHitPos.xz, 0.01, WATER_DEPTH);

             // smooth the normal with distance to avoid disturbing high frequency noise
             N = mix(N, vec3(0.0, 1.0, 0.0), 0.8 * min(1.0, sqrt(dist * 0.01) * 1.1));

             // calculate fresnel coefficient
             float fresnel = (0.04 + (1.0 - 0.04) * (pow(1.0 - max(0.0, dot(-N, rayDirection)), 5.0)));

             // reflect the ray and make sure it bounces up
             vec3 R = normalize(reflect(rayDirection, N));
             R.y = abs(R.y);

             // calculate the reflection and approximate subsurface scattering
             vec3 reflection = getAtmosphere(R) + getSun(R) * pc.sunColor.rgb;
             vec3 scattering = pc.voidColor.rgb * 0.1 * (0.2 + (waterHitPos.y + WATER_DEPTH) / WATER_DEPTH);

             // return the combined result
             waterColor = fresnel * reflection + scattering;
         }
        }
        vec2 uv = newTexCoords;
        uv -= 0.5;
        uv.y = -uv.y;


        const float smoothCurve = mix(0.0, 0.45, smoothstep(-0.5, 0.5, uv.y));
        const float curve = -(1.0 - dot(uv, uv) * smoothCurve);
        vec2 fragCoords = newTexCoords * vec2(texSize);
        const vec3 ro = vec3(camera.cameraPosition.x, 0.0, camera.cameraPosition.z);
        color = Sky(ro, rayDirection, waterColor);

        color += bloom;
    }
    
    color = mix(color, volumetricSample.rgb, volumetricSample.a * 0.3);

    if (paletteEnabled)
    {
        color = ComputeQuantizedColor(color, pc.ditherAmount, pc.paletteAmount);
    }

    switch (pc.tonemappingFunction)
    {
        case ACES: color = aces(color); break;
        case AGX: color = agx(color); break;
        case FILMIC: color = filmic(color); break;
        case LOTTES: color = lottes(color); break;
        case NEUTRAL: color = neutral(color); break;
        case REINHARD: color = reinhard(color); break;
        case REINHARD2: color = reinhard2(color); break;
        case UCHIMURA: color = uchimura(color); break;
        case UNCHARTED2: color = uncharted2(color); break;
        case UNREAL: color = unreal(color); break;
    }

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    if (toneAdjustmentsEnabled)
    {
        color = (SaturationMatrix(pc.saturation) * vec4(color, 1.0)).rgb;
        BrightnessAdjust(color, pc.brightness);
        ContrastAdjust(color, pc.contrast);
        color = Vibrance(color, pc.vibrance);
        color = ShiftHue(color, pc.hue);
    }

    if (vignetteEnabled)
    {
        color = Vignette(color, texCoords, pc.vignetteIntensity);
    }

    color = ScreenFlash(color, pc.flashColor.rgb, texCoords, pc.flashColor.a);


    outColor = vec4(color, 1.0);
}




float noise(in vec2 uv)
{
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    f = f * f * (3. - 2. * f);

    float lb = hashwithoutsine12(i + vec2(0., 0.));
    float rb = hashwithoutsine12(i + vec2(1., 0.));
    float lt = hashwithoutsine12(i + vec2(0., 1.));
    float rt = hashwithoutsine12(i + vec2(1., 1.));

    return mix(mix(lb, rb, f.x),
               mix(lt, rt, f.x), f.y);
}

#define OCTAVES 8
float fbm(in vec2 uv)
{
    float value = 0.0;
    float amplitude = .5;

    for (int i = 0; i < OCTAVES; i++)
    {
        value += noise(uv) * amplitude;

        amplitude *= .5;

        uv *= 2.;
    }

    return value;
}

vec3 Sky(in vec3 ro, in vec3 rd, in vec3 waterColor)
{
    const float SC = 1e5;

    // Calculate sky plane
    float dist = (SC - ro.y) / rd.y;
    vec2 p = (ro + dist * rd).xz;
    p *= 4.4 / SC;

    // from iq's shader, https://www.shadertoy.com/view/MdX3Rr
    vec3 lightDir = normalize(scene.directionalLight.direction.xyz); //sunDirection
    float sundot = clamp(dot(rd, lightDir), 0.0, 1.0);

    vec3 cloudCol = pc.cloudsColor.rgb;
    vec3 skyCol = pc.skyColor.rgb - rd.y * .2 * vec3(1., .5, 1.) + .15 * .5;
    //vec3 skyCol = pc.skyColor.rgb - rd.y * rd.y * 0.5;
    skyCol = mix(skyCol, 0.85 * pc.skyColor.rgb, pow(1.0 - max(rd.y, 0.0), 4.0));

    // sun
    vec3 sun = 0.2 * pc.sunColor.rgb * pow(sundot, 8.0);
    sun += 0.95 * pc.sunColor.rgb * pow(sundot, 2048.0);
    sun += 0.2 * pc.sunColor.rgb * pow(sundot, 128.0);
    sun = clamp(sun, 0.0, 1.0);
    skyCol += sun;

    // clouds
    float t = pc.time * pc.cloudsSpeed;
    float den = fbm(vec2(p.x - t, p.y - t));
    skyCol = mix(skyCol, cloudCol, smoothstep(.4, .8, den));


    // horizon (still optional—keep if you want some blending)
    skyCol = mix(skyCol, waterColor, pow(1.0 - max(rd.y, 0.0), 16.0));

    // HORIZON FOG — applies to everything equally
    vec3 fogColor = pc.voidColor.rgb; // or your choice
    float horizonFogAmount = 1.0 - smoothstep(0.0, 0.2, abs(rd.y));
    skyCol = mix(skyCol, fogColor, horizonFogAmount);

    return skyCol;
}

vec3 SaturateColor(vec3 color, float saturationFactor)
{
    // Convert to "gray" by averaging
    float gray = (color.r + color.g + color.b) / 3.0;
    // Interpolate between gray and the original color
    return mix(vec3(gray), color, saturationFactor);
}

vec3 ComputeQuantizedColor(vec3 color, float ditherAmount, float blendFactor)
{
    const float ditherValue = ((bayer[int(gl_FragCoord.x) % 4][int(gl_FragCoord.y) % 4] + 0.5) / 16.0) - 0.5;
    color += ditherValue * ditherAmount;

    // Snap to the closest color from the palette
    float bestDistance = 1000.0;
    vec3 bestColor = vec3(0.0);
    for (int i = 0; i < pc.paletteSize; i++) {
        float d = distance(color, palette[i].rgb);
        if (d < bestDistance) {
            bestDistance = d;
            bestColor = SaturateColor(palette[i].rgb, 1.2);
        }
    }

    // Blend the color with the best color from the palette
    color = mix(color, bestColor, blendFactor);
    return color;
}

vec2 ComputePixelatedUV(float depthSample, float levels, float minPixelSize, float maxPixelSize, vec2 texCoords, vec2 screenSize)
{
/**
    // Clamp and quantize the depth sample to one of the discrete levels.
    float t = clamp(depthSample * pc.pixelizationDepthBias, 0.0, 1.0);
    t = floor(t * levels) / (levels - 1.0);

    // Now use the quantized value in the mix function.
    const float diameter = mix(pc.minPixelSize, pc.maxPixelSize, t);

    // Compute "pixelated" (stepped) texture coordinates using the floor() function.
    // The position is adjusted to match the circles, i.e. so a pixelated block is at the center of the
    // display.
    const vec2 count = screenSize.xy / diameter;
    const vec2 shift = vec2(0.5) - fract(count / 2.0);
    vec2 uv = floor(count * texCoords + shift) / count;

    // Sample the texture, using an offset to the center of the pixelated block.
    // NOTE: Use a large negative bias to effectively disable mipmapping, which would otherwise lead
    // to sampling artifacts where the UVs change abruptly at the pixelated block boundaries.
    uv += vec2(0.5) / count;
    uv = clamp(uv, 0.0, 0.99);
    return uv;

*/


    // Clamp and quantize the depth sample to one of the discrete levels.


    float inetrpolationValue = min(1.0, depthSample * pc.pixelizationDepthBias);
    float granularity = mix(minPixelSize, maxPixelSize, inetrpolationValue);
    granularity = round(clamp(granularity, minPixelSize, maxPixelSize));
    float dx = granularity / screenSize.x;
    float dy = granularity / screenSize.y;
    vec2 uv = vec2(dx * (floor(texCoords.x / dx) + 0.5),
    dy * (floor(texCoords.y / dy) + 0.5));

    return uv;

}


vec3 Vignette(in vec3 color, in vec2 uv, in float intensity)
{
    uv *= 1.0 - uv.yx;
    float vig = uv.x * uv.y * 15;
    return color * pow(vig, intensity);
}

vec3 ScreenFlash(in vec3 color, in vec3 flashColor, in vec2 uv, in float intensity)
{
    vec2 vignetteUV = uv * (1.0 - uv.yx);

    float vignetteShape = vignetteUV.x * vignetteUV.y;


    float flashFactor = clamp(1.0 - pow(vignetteShape * 16.0, intensity), 0.0, 1.0);

    return mix(color, flashColor, flashFactor);
}

vec2 LensDistortionUV(vec2 uv, float k, float kCube) {

    vec2 t = uv - .5;
    float r2 = t.x * t.x + t.y * t.y;
    float f = 0.;

    if (kCube == 0.0) {
        f = 1. + r2 * k;
    } else {
        f = 1. + r2 * (k + kCube * sqrt(r2));
    }

    vec2 nUv = f * t + .5;

    return nUv;

}

void BrightnessAdjust(inout vec3 color, in float b)
{
    color += b;
}

void ContrastAdjust(inout vec3 color, in float c)
{
    float t = 0.5 - c * 0.5;
    color = color * c + t;
}

mat4 SaturationMatrix(float saturation)
{
    vec3 luminance = vec3(0.3086, 0.6094, 0.0820);
    float oneMinusSat = 1.0 - saturation;
    vec3 red = vec3(luminance.x * oneMinusSat);
    red.r += saturation;

    vec3 green = vec3(luminance.y * oneMinusSat);
    green.g += saturation;

    vec3 blue = vec3(luminance.z * oneMinusSat);
    blue.b += saturation;

    return mat4(
    red, 0,
    green, 0,
    blue, 0,
    0, 0, 0, 1);
}

int Modi(int x, int y) {
    return x - y * (x / y);
}

int And(int a, int b) {
    int result = 0;
    int n = 1;
    const int BIT_COUNT = 32;

    for (int i = 0; i < BIT_COUNT; i++) {
        if ((Modi(a, 2) == 1) && (Modi(b, 2) == 1)) {
            result += n;
        }

        a >>= 1;
        b >>= 1;
        n <<= 1;

        if (!(a > 0 && b > 0))
        break;
    }
    return result;
}

// forked from https://www.shadertoy.com/view/llGSzK
// performance optimized by Ruofei
vec3 Vibrance(vec3 inCol, float vibrance) //r,g,b 0.0 to 1.0,  vibrance 1.0 no change, 0.0 image B&W.
{
    vec3 outCol;
    if (vibrance <= 1.0)
    {
        float avg = dot(inCol, vec3(0.3, 0.6, 0.1));
        outCol = mix(vec3(avg), inCol, vibrance);
    }
    else // vibrance > 1.0
    {
        float hue_a, a, f, p1, p2, p3, i, h, s, v, amt, _max, _min, dlt;
        float br1, br2, br3, br4, br5, br2_or_br1, br3_or_br1, br4_or_br1, br5_or_br1;
        int use;

        _min = min(min(inCol.r, inCol.g), inCol.b);
        _max = max(max(inCol.r, inCol.g), inCol.b);
        dlt = _max - _min + 0.00001 /*Hack to fix divide zero infinities*/;
        h = 0.0;
        v = _max;

        br1 = step(_max, 0.0);
        s = (dlt / _max) * (1.0 - br1);
        h = -1.0 * br1;

        br2 = 1.0 - step(_max - inCol.r, 0.0);
        br2_or_br1 = max(br2, br1);
        h = ((inCol.g - inCol.b) / dlt) * (1.0 - br2_or_br1) + (h * br2_or_br1);

        br3 = 1.0 - step(_max - inCol.g, 0.0);

        br3_or_br1 = max(br3, br1);
        h = (2.0 + (inCol.b - inCol.r) / dlt) * (1.0 - br3_or_br1) + (h * br3_or_br1);

        br4 = 1.0 - br2 * br3;
        br4_or_br1 = max(br4, br1);
        h = (4.0 + (inCol.r - inCol.g) / dlt) * (1.0 - br4_or_br1) + (h * br4_or_br1);

        h = h * (1.0 - br1);

        hue_a = abs(h); // between h of -1 and 1 are skin tones
        a = dlt; // Reducing enhancements on small rgb differences

        // Reduce the enhancements on skin tones.
        a = step(1.0, hue_a) * a * (hue_a * 0.67 + 0.33) + step(hue_a, 1.0) * a;
        a *= (vibrance - 1.0);
        s = (1.0 - a) * s + a * pow(s, 0.25);

        i = floor(h);
        f = h - i;

        p1 = v * (1.0 - s);
        p2 = v * (1.0 - (s * f));
        p3 = v * (1.0 - (s * (1.0 - f)));

        inCol = vec3(0.0);
        i += 6.0;
        //use = 1 << ((int)i % 6);
        use = int(pow(2.0, mod(i, 6.0)));
        a = float(And(use, 1)); // i == 0;
        use >>= 1;
        inCol += a * vec3(v, p3, p1);

        a = float(And(use, 1)); // i == 1;
        use >>= 1;
        inCol += a * vec3(p2, v, p1);

        a = float(And(use, 1)); // i == 2;
        use >>= 1;
        inCol += a * vec3(p1, v, p3);

        a = float(And(use, 1)); // i == 3;
        use >>= 1;
        inCol += a * vec3(p1, p2, v);

        a = float(And(use, 1)); // i == 4;
        use >>= 1;
        inCol += a * vec3(p3, p1, v);

        a = float(And(use, 1)); // i == 5;
        use >>= 1;
        inCol += a * vec3(v, p1, p2);

        outCol = inCol;
    }
    return outCol;
}

// remixed from mAlk's https://www.shadertoy.com/view/MsjXRt
vec3 ShiftHue(in vec3 col, in float Shift)
{
    vec3 P = vec3(0.55735) * dot(vec3(0.55735), col);
    vec3 U = col - P;
    vec3 V = cross(vec3(0.55735), U);
    col = U * cos(Shift * 6.2832) + V * sin(Shift * 6.2832) + P;
    return col;
}
