#define MAX_POINT_LIGHTS 8192

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
    float attenuation;
};

struct PointLightArray
{
    PointLight lights[MAX_POINT_LIGHTS];
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
    vec3 fogColor;
    float fogDensity;
    float fogHeight;
};

struct Instance
{
    mat4 model;
    uint materialIndex;
    float boundingRadius;
    uint boneOffset;
    bool isStaticDraw;
    bool padding[3];
};

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


