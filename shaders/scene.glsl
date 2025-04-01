#define MAX_POINT_LIGHTS 8192
#define MAX_DECALS 32

struct Camera
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 skydomeMVP;
    mat4 inverseView;
    mat4 inverseProj;
    mat4 inverseVP;
    vec3 cameraPosition;
    int distanceCullingEnabled;
    vec4 frustum;
    float zNear;
    float zFar;
    int cullingEnabled;
    int projectionType;

    vec2 _padding;
};

struct DirectionalLight
{
    mat4 lightVP;
    mat4 depthBiasMVP;

    vec4 direction;
    vec4 color;
    float poissonWorldOffset;
    float poissonConstant;
};

struct PointLight
{
    vec3 position;
    float range;
    vec3 color;
    float intensity;
};

struct PointLightArray
{
    PointLight lights[MAX_POINT_LIGHTS];
    uint count;
};

struct Decal
{
    mat4 invModel;
    vec3 orientation;
    uint albedoIndex;
};

struct DecalArray
{
    Decal decals[MAX_DECALS];
    uint count;
};

struct Scene
{
    DirectionalLight directionalLight;

    uint irradianceIndex;
    uint prefilterIndex;
    uint brdfLUTIndex;
    uint staticShadowMapIndex;

    uint dynamicShadowMapIndex;
    float fogDensity;
    vec3 fogColor;
};

struct Instance
{
    mat4 model;
    uint materialIndex;
    float boundingRadius;
    uint boneOffset;
    bool isStaticDraw;
    float transparency;
    bool padding[2];
};

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

float randomIndex(vec3 seed, int i) {
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

vec3 ReconstructViewPosition(in float depth, in vec2 screenUv, in mat4 invProj)
{
    vec2 ndc = screenUv * 2.0 - 1.0;
    vec4 clipSpacePos = vec4(ndc, depth, 1.0);
    vec4 viewSpacePos = invProj * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;

    return viewSpacePos.xyz;
}

vec3 ReconstructWorldPosition(in float depth, in vec2 screenUv, in mat4 invVP)
{
    vec2 ndc = screenUv * 2.0 - 1.0;
    vec4 clipSpacePos = vec4(ndc, depth, 1.0);
    vec4 worldSpacePos = invVP * clipSpacePos;
    worldSpacePos /= worldSpacePos.w;

    return worldSpacePos.xyz;
}

const uint ROUGHNESS_BITS = 5u;
const uint METALLIC_BITS = 3u;
const uint ROUGHNESS_RANGE = (1u << ROUGHNESS_BITS) - 1u;
const uint METALLIC_RANGE = (1u << METALLIC_BITS) - 1u;

float EncodeRM(float roughness, float metallic)
{
    uint r = uint(roughness * ROUGHNESS_RANGE) & ROUGHNESS_RANGE;
    uint m = uint(metallic * METALLIC_RANGE) & METALLIC_RANGE;
    uint c = (r << METALLIC_BITS) | m;
    return float(c) * (1.0 / 255.0);
}

void DecodeRM(float pack, out float roughness, out float metallic)
{
    uint c = uint(pack * 255.0);
    uint r = (c >> METALLIC_BITS) & ROUGHNESS_RANGE;
    uint m = c & METALLIC_RANGE;
    roughness = float(r) / ROUGHNESS_RANGE;
    metallic = float(m) / METALLIC_RANGE;
}


