#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"
#include "settings.glsl"
#include "tonemapping.glsl"

layout (push_constant) uniform PushConstants
{
    uint hdrTargetIndex;
    uint bloomTargetIndex;

    uint tonemappingFunction;
    float exposure;

    bool enableVignette;
    float vignetteIntensity;

    bool enableLensDistortion;
    float lensDistortionIntensity;
    float lensDistortionCubicIntensity;
    float screenScale;

    bool enableToneAdjustments;
    float brightness;
    float contrast;
    float saturation;
    float vibrance;
    float hue;
} pc;

layout (set = 1, binding = 0) uniform BloomSettingsUBO
{
    BloomSettings bloomSettings;
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

void main()
{
    vec2 newTexCoords = texCoords;
    if (pc.enableLensDistortion)
    {
        newTexCoords = LensDistortionUV(texCoords, pc.lensDistortionIntensity, pc.lensDistortionCubicIntensity);
        newTexCoords = (newTexCoords - 0.5) * pc.screenScale + 0.5;
    }

    vec3 hdrColor = texture(bindless_color_textures[nonuniformEXT(pc.hdrTargetIndex)], newTexCoords).rgb;

    vec3 bloomColor = texture(bindless_color_textures[nonuniformEXT(pc.bloomTargetIndex)], newTexCoords).rgb;
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

    if (pc.enableToneAdjustments)
    {
        color = (SaturationMatrix(pc.saturation) * vec4(color, 1.0)).rgb;
        BrightnessAdjust(color, pc.brightness);
        ContrastAdjust(color, pc.contrast);
        color = Vibrance(color, pc.vibrance);
        color = ShiftHue(color, pc.hue);
    }

    if (pc.enableVignette)
    {
        color = Vignette(color, texCoords, pc.vignetteIntensity);
    }

    outColor = vec4(color, 1.0);
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
        a = dlt;      // Reducing enhancements on small rgb differences

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
