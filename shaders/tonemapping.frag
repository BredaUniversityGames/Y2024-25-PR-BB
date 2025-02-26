#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"
#include "tonemapping.glsl"
#include "scene.glsl"


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

    uint tonemappingFunction;
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

    //pixelization
    float minPixelSize;
    float maxPixelSize;
    float pixelizationLevels;
    float pixelizationDepthBias;
    uint screenWidth;
    uint screenHeight;

    float ditherAmount;
    float paletteAmount;
    float time;
    vec4 palette[5];
} pc;

layout (set = 1, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
};
layout (set = 2, binding = 0) uniform CameraUBO
{
    Camera camera;
};

layout (location = 0) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

vec3 Vignette(in vec3 color, in vec2 texCoords, in float intensity);
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

// 4x4 Bayer matrix with values 0..15
const float bayer[4][4] = float[4][4](
float[4](0.0, 8.0, 2.0, 10.0),
float[4](12.0, 4.0, 14.0, 6.0),
float[4](3.0, 11.0, 1.0, 9.0),
float[4](15.0, 7.0, 13.0, 5.0)
);


float Rain(in vec2 fragCoord, in float time)
{
    // cell size in pixels (controls raindrop density)
    float cellSize = 50.0;
    // rain speed in pixels per second
    float rainSpeed = 200.0;
    // Determine which grid cell we're in and the fractional position within that cell
    vec2 cell = floor(fragCoord / cellSize);
    vec2 uvCell = fract(fragCoord / cellSize);
    // Generate a random value per cell for a different drop offset
    float randVal = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
    // Compute the drop's vertical position in the cell (animated by time)
    float dropPos = fract(randVal - (time * rainSpeed / cellSize));
    // How wide (in cell fraction) the drop is
    float dropWidth = 0.1;
    // Intensity is high when the fragment’s y coordinate (within the cell) is near the dropPos
    float intensity = 1.0 - smoothstep(0.0, dropWidth, abs(uvCell.y - dropPos));
    return intensity;
}
float hash12(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

#define RAIN_DENSITY .00006      // density of drop
#define BRIGTHNESS  .9        // raindrop brightness contrast
#define BLUR_LENGTH 20.0       // max length of raindrop blured line
#define SPEED 0.5


#define rnd(p, s)   fract(sin(((p) + .01 * (s)) * 12.9898) * 43758.5453)


float linearize_depth(float d, float zNear, float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
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
    float depthSample = texture(bindless_depth_textures[nonuniformEXT (pc.depthIndex)], texCoords).r;

    vec3 hdrColor = vec3(0.0);
    if (pixelizationEnabled)
    {
        const vec2 uv = ComputePixelatedUV(depthSample, pc.pixelizationLevels, pc.minPixelSize, pc.maxPixelSize, newTexCoords, vec2(pc.screenWidth, pc.screenHeight));
        hdrColor = texture(bindless_color_textures[nonuniformEXT (pc.hdrTargetIndex)], uv, -32.0).rgb;
    } else
    {
        hdrColor = texture(bindless_color_textures[nonuniformEXT(pc.hdrTargetIndex)], newTexCoords).rgb;
    }


    if (paletteEnabled)
    {
        hdrColor = ComputeQuantizedColor(hdrColor, pc.ditherAmount, pc.paletteAmount);
    }

/**
// --- Begin Rain Effect (Screen-space) ---
    // Use the fragment coordinate normalized by screen resolution.
    vec2 q = texCoords;

    float dis = 1.0;
    for (int i = 0; i < 12; i++)
    {
        vec3 plane = camera.cameraPosition + vec3(0.0, 0.0, 1.0) * dis;
        // Scale factor to vary the drop spacing with layer distance.
        //if (plane.z < depthSample)
        {
            float f = pow(dis, 0.45) + 0.25;
            // Create a shifting coordinate based on screen coord, time and layer.
            vec2 st = f * (q * vec2(1.5, 0.05) + vec2(-pc.time * 0.1 + q.y * 0.5, pc.time * 0.12));
            // Instead of sampling a noise texture, sum two hash values at different scales.
            f = hash12(st * 0.5) + hash12(st * 0.284);
            // Sharpen the noise into streaks (the exponent creates very tight peaks).
            f = clamp(pow(abs(f * 0.5), 29.0) * 140.0, 0.0, q.y * 0.4 + 0.05);
            // A constant brightness for the rain streak.
            vec3 rain = vec3(0.25);
            hdrColor += rain * f;
        }

        dis += 3.5;
    }*/
    vec2 R = vec2(pc.screenWidth, pc.screenHeight);
    vec2 U = gl_FragCoord.xy;

    // Define the world gravity (rain falls downward in world space)
    vec3 worldGravity = vec3(0.0, -1.0, 0.0);
    // Transform gravity into view space (rotation only)
    vec3 viewGravity = mat3(camera.view) * worldGravity;

    // Compute a tilt factor that goes from 0 (when viewGravity.y is 1.0 in magnitude, i.e. camera level)
    // to 1 (when viewGravity.y is 0, i.e. extreme tilt)
    float tilt = 1.0 - clamp(abs(viewGravity.y), 0.0, 1.0);
    // Compute the drop stretch: when tilt is 0, dropStretch is 1; as tilt increases, so does dropStretch.
    // Adjust the multiplier (here 2.0) to control how pronounced the effect is.
    float dropStretch = 1.0 + tilt * 20.0;



    // Extract the camera's right vector from the view matrix.
    vec3 right = vec3(camera.view[0][0], camera.view[1][0], camera.view[2][0]);
    // Compute the camera yaw angle. This angle represents the left/right rotation.
    float camYaw = atan(right.z, right.x);
    // Apply a scale factor to determine how much the rain pattern should shift to cancel the yaw.
    // (Adjust the 50.0 multiplier until it feels right in your scene.)
    float rotationOffset = camYaw * 50.0;
    U.x += rotationOffset;

    float Ny = RAIN_DENSITY * R.y; // number of drop per column

    vec3 auxHdrColor = hdrColor;

    float layerDisplacement = 1.0f;
    //depthSample = linearize_depth(depthSample, camera.zNear, camera.zFar);
    //layers
    for (float l = 0.; l < 4.; l++)
    {
        vec2 worldOffset = - vec2(camera.cameraPosition.x, camera.cameraPosition.z) * 0.001f;
        U.x += l * 1.2;
        U.x -= worldOffset.x;
        layerDisplacement -= worldOffset.y;
        for (float i = 0.; i <= floor(Ny); i++) {
            // to deal with more than one drop per column
            float y = floor(mod(rnd(U.x, 2. * i) * R.y + (SPEED * pc.time), R.y)); // drop altitude
            if (rnd(U.x, 2. * i + 1.) < (Ny - i) && abs(U.y - y) < ((BLUR_LENGTH * layerDisplacement) / dropStretch) * U.x / R.x)
            {
                if (depthSample * 20 < layerDisplacement - 1.0)
                {
                    hdrColor += BRIGTHNESS; //  / (U.x/R.x); // variant: keep total drop brightness. attention: saturated on the left 5%

                } else
                {
                    hdrColor = auxHdrColor;
                }

            }
        }

        layerDisplacement += 0.25f;
    }


    hdrColor += bloomColor * bloomSettings.strength;

    vec3 color = vec3(1.0) - exp(-hdrColor * pc.exposure);

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



    outColor = vec4(color, 1.0);
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
    for (int i = 0; i < 5; i++) {
        float d = distance(color, pc.palette[i].rgb);
        if (d < bestDistance) {
            bestDistance = d;
            bestColor = SaturateColor(pc.palette[i].rgb, 1.2);
        }
    }

    // Blend the color with the best color from the palette
    color = mix(color, bestColor, blendFactor);
    return color;
}

vec2 ComputePixelatedUV(float depthSample, float levels, float minPixelSize, float maxPixelSize, vec2 texCoords, vec2 screenSize)
{
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
}


vec3 Vignette(in vec3 color, in vec2 uv, in float intensity)
{
    uv *= 1.0 - uv.yx;
    float vig = uv.x * uv.y * 15;
    return color * pow(vig, intensity);
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
