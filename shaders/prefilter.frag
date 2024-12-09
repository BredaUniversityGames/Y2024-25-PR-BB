#version 460
#extension GL_EXT_nonuniform_qualifier: enable

#include "bindless.glsl"

layout (location = 0) in vec2 texCoords;

layout (push_constant) uniform PushConstants
{
    uint index;
    float roughness;
    uint hdriIndex;
} pc;

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265359;
#define MIP_CLAMP 3



vec3 MapDirection(vec2 coords, uint faceIndex);
vec2 SampleSphericalMap(vec3 dir);
float RadicalInverse_VdC(uint bits);
vec2 Hammersley(uint i, uint N);
vec3 ImportantceSampleGGX(vec2 Xi, vec3 N, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);

void main()
{
    vec3 direction = MapDirection(texCoords, pc.index);

    vec3 N = normalize(direction);
    vec3 V = N;
    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 1.0;
    vec3 prefilteredColor = vec3(0.0);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportantceSampleGGX(Xi, N, pc.roughness);
        vec3 L = reflect(-V, H);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0)
        {

            float D   = DistributionGGX(N, H, pc.roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float resolution = 512.0;// resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);


            float mipLevel = pc.roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            vec3 sampleColor = clamp(texture(bindless_color_textures[nonuniformEXT(pc.hdriIndex)], SampleSphericalMap(L)).rgb, 0, MIP_CLAMP);

            prefilteredColor += sampleColor * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor /= totalWeight;

    outColor = vec4(prefilteredColor, 1.0);
}

vec3 MapDirection(vec2 coords, uint faceIndex)
{
    vec2 uvRemapped = coords * 2.0 - 1.0;
    vec3 direction;
    if (faceIndex == 0)
    { // +X face
        direction = vec3(1.0, -uvRemapped.y, -uvRemapped.x);
    }
    else if (faceIndex == 1)
    { // -X face
        direction = vec3(-1.0, -uvRemapped.y, uvRemapped.x);
    }
    else if (faceIndex == 2)
    { // +Y face
        direction = vec3(uvRemapped.x, 1.0, uvRemapped.y);
    }
    else if (faceIndex == 3)
    { // -Y face
        direction = vec3(uvRemapped.x, -1.0, -uvRemapped.y);
    }
    else if (faceIndex == 4)
    { // +Z face
        direction = vec3(uvRemapped.x, -uvRemapped.y, 1.0);
    }
    else if (faceIndex == 5)
    { // -Z face
        direction = vec3(-uvRemapped.x, -uvRemapped.y, -1.0);
    }

    return normalize(direction);
}

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;// / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportantceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}