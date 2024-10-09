struct Camera
{
    mat4 VP;
    mat4 view;
    mat4 proj;
    mat4 skydomeMVP;
    vec3 cameraPosition;
    int distCull;
    vec4 frustum;
    float zNear;
    float zFar;
    int cullingEnabled;

    vec3 _padding;
};