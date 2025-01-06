#define MAX_POINT_LIGHTS 8192

struct Camera
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 invProj;
    mat4 skydomeMVP;
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
};

struct Instance
{
    mat4 model;
    uint materialIndex;
    float boundingRadius;
    uint boneOffset;
};