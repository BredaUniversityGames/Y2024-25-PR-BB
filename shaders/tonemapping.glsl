#define ACES 0
#define AGX 1
#define FILMIC 2
#define LOTTES 3
#define NEUTRAL 4
#define REINHARD 5
#define REINHARD2 6
#define UCHIMURA 7
#define UNCHARTED2 8
#define UNREAL 9

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float aces(float x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Missing Deadlines (Benjamin Wrensch): https://iolite-engine.com/blog_posts/minimal_agx_implementation
// Filament: https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp#L263
// https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBaseRec2020.py

// Three.js: https://github.com/mrdoob/three.js/blob/4993e3af579a27cec950401b523b6e796eab93ec/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js#L79-L89
// Matrices for rec 2020 <> rec 709 color space conversion
// matrix provided in row-major order so it has been transposed
// https://www.itu.int/pub/R-REP-BT.2407-2017
const mat3 LINEAR_REC2020_TO_LINEAR_SRGB = mat3(
1.6605, -0.1246, -0.0182,
-0.5876, 1.1329, -0.1006,
-0.0728, -0.0083, 1.1187
);

const mat3 LINEAR_SRGB_TO_LINEAR_REC2020 = mat3(
0.6274, 0.0691, 0.0164,
0.3293, 0.9195, 0.0880,
0.0433, 0.0113, 0.8956
);

// Converted to column major from blender: https://github.com/blender/blender/blob/fc08f7491e7eba994d86b610e5ec757f9c62ac81/release/datafiles/colormanagement/config.ocio#L358
const mat3 AgXInsetMatrix = mat3(
0.856627153315983, 0.137318972929847, 0.11189821299995,
0.0951212405381588, 0.761241990602591, 0.0767994186031903,
0.0482516061458583, 0.101439036467562, 0.811302368396859
);

// Converted to column major and inverted from https://github.com/EaryChow/AgX_LUT_Gen/blob/ab7415eca3cbeb14fd55deb1de6d7b2d699a1bb9/AgXBaseRec2020.py#L25
// https://github.com/google/filament/blob/bac8e58ee7009db4d348875d274daf4dd78a3bd1/filament/src/ToneMapper.cpp#L273-L278
const mat3 AgXOutsetMatrix = mat3(
1.1271005818144368, -0.1413297634984383, -0.14132976349843826,
-0.11060664309660323, 1.157823702216272, -0.11060664309660294,
-0.016493938717834573, -0.016493938717834257, 1.2519364065950405
);

const float AgxMinEv = -12.47393;
const float AgxMaxEv = 4.026069;

// 0: Default, 1: Golden, 2: Punchy
#ifndef AGX_LOOK
  #define AGX_LOOK 0
#endif

vec3 agxAscCdl(vec3 color, vec3 slope, vec3 offset, vec3 power, float sat) {
    const vec3 lw = vec3(0.2126, 0.7152, 0.0722);
    float luma = dot(color, lw);
    vec3 c = pow(color * slope + offset, power);
    return luma + sat * (c - luma);
}

// Sample usage
vec3 agx(vec3 color) {
    color = LINEAR_SRGB_TO_LINEAR_REC2020 * color; // From three.js

    // 1. agx()
    // Input transform (inset)
    color = AgXInsetMatrix * color;

    color = max(color, 1e-10); // From Filament: avoid 0 or negative numbers for log2

    // Log2 space encoding
    color = clamp(log2(color), AgxMinEv, AgxMaxEv);
    color = (color - AgxMinEv) / (AgxMaxEv - AgxMinEv);

    color = clamp(color, 0.0, 1.0); // From Filament

    // Apply sigmoid function approximation
    // Mean error^2: 3.6705141e-06
    vec3 x2 = color * color;
    vec3 x4 = x2 * x2;
    color = + 15.5 * x4 * x2
    - 40.14 * x4 * color
    + 31.96 * x4
    - 6.868 * x2 * color
    + 0.4298 * x2
    + 0.1191 * color
    - 0.00232;

    // 2. agxLook()
    #if AGX_LOOK == 1
    // Golden
    color = agxAscCdl(color, vec3(1.0, 0.9, 0.5), vec3(0.0), vec3(0.8), 1.3);
    #elif AGX_LOOK == 2
    // Punchy
    color = agxAscCdl(color, vec3(1.0), vec3(0.0), vec3(1.35), 1.4);
    #endif

    // 3. agxEotf()
    // Inverse input transform (outset)
    color = AgXOutsetMatrix * color;

    // sRGB IEC 61966-2-1 2.2 Exponent Reference EOTF Display
    // NOTE: We're linearizing the output here. Comment/adjust when
    // *not* using a sRGB render target
    color = pow(max(vec3(0.0), color), vec3(2.2)); // From filament: max()

    color = LINEAR_REC2020_TO_LINEAR_SRGB * color; // From three.js
    // Gamut mapping. Simple clamp for now.
    color = clamp(color, 0.0, 1.0);

    return color;
}

// Filmic Tonemapping Operators http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 filmic(vec3 x) {
    vec3 X = max(vec3(0.0), x - 0.004);
    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, vec3(2.2));
}

float filmic(float x) {
    float X = max(0.0, x - 0.004);
    float result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, 2.2);
}

// Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
vec3 lottes(vec3 x) {
    const vec3 a = vec3(1.6);
    const vec3 d = vec3(0.977);
    const vec3 hdrMax = vec3(8.0);
    const vec3 midIn = vec3(0.18);
    const vec3 midOut = vec3(0.267);

    const vec3 b =
    (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const vec3 c =
    (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(x, a) / (pow(x, a * d) * b + c);
}

float lottes(float x) {
    const float a = 1.6;
    const float d = 0.977;
    const float hdrMax = 8.0;
    const float midIn = 0.18;
    const float midOut = 0.267;

    const float b =
    (-pow(midIn, a) + pow(hdrMax, a) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);
    const float c =
    (pow(hdrMax, a * d) * pow(midIn, a) - pow(hdrMax, a) * pow(midIn, a * d) * midOut) /
    ((pow(hdrMax, a * d) - pow(midIn, a * d)) * midOut);

    return pow(x, a) / (pow(x, a * d) * b + c);
}

// Khronos PBR Neutral Tone Mapper
// https://github.com/KhronosGroup/ToneMapping/tree/main/PBR_Neutral

// Input color is non-negative and resides in the Linear Rec. 709 color space.
// Output color is also Linear Rec. 709, but in the [0, 1] range.
vec3 neutral(vec3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, vec3(newPeak), g);
}

vec3 reinhard(vec3 x) {
    return x / (1.0 + x);
}

float reinhard(float x) {
    return x / (1.0 + x);
}

vec3 reinhard2(vec3 x) {
    const float L_white = 4.0;

    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

float reinhard2(float x) {
    const float L_white = 4.0;

    return (x * (1.0 + x / (L_white * L_white))) / (1.0 + x);
}

// Uchimura 2017, "HDR theory and practice"
// Math: https://www.desmos.com/calculator/gslcdxvipg
// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
vec3 uchimura(vec3 x, float P, float a, float m, float l, float c, float b) {
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
    vec3 w2 = vec3(step(m + l0, x));
    vec3 w1 = vec3(1.0 - w0 - w2);

    vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
    vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
    vec3 L = vec3(m + a * (x - m));

    return T * w0 + L * w1 + S * w2;
}

vec3 uchimura(vec3 x) {
    const float P = 1.0; // max display brightness
    const float a = 1.0; // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4; // linear section length
    const float c = 1.33; // black
    const float b = 0.0; // pedestal

    return uchimura(x, P, a, m, l, c, b);
}

float uchimura(float x, float P, float a, float m, float l, float c, float b) {
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = m * pow(x / m, c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

float uchimura(float x) {
    const float P = 1.0; // max display brightness
    const float a = 1.0; // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4; // linear section length
    const float c = 1.33; // black
    const float b = 0.0; // pedestal

    return uchimura(x, P, a, m, l, c, b);
}

vec3 uncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 uncharted2(vec3 color) {
    const float W = 11.2;
    float exposureBias = 2.0;
    vec3 curr = uncharted2Tonemap(exposureBias * color);
    vec3 whiteScale = 1.0 / uncharted2Tonemap(vec3(W));
    return curr * whiteScale;
}

float uncharted2Tonemap(float x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float uncharted2(float color) {
    const float W = 11.2;
    const float exposureBias = 2.0;
    float curr = uncharted2Tonemap(exposureBias * color);
    float whiteScale = 1.0 / uncharted2Tonemap(W);
    return curr * whiteScale;
}

// Unreal 3, Documentation: "Color Grading"
// Adapted to be close to Tonemap_ACES, with similar range
// Gamma 2.2 correction is baked in, don't use with sRGB conversion!
vec3 unreal(vec3 x) {
    return x / (x + 0.155) * 1.019;
}

float unreal(float x) {
    return x / (x + 0.155) * 1.019;
}
