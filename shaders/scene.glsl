struct Camera
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 skydomeMVP;
    vec3 cameraPosition;
    float _padding;
};

struct DirectionalLight
{
    mat4 lightVP;
    mat4 depthBiasMVP;

    vec4 direction;
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
};