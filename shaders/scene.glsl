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
    uint shadowMapIndex;

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
