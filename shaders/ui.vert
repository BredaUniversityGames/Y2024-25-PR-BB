#version 450 

vec2 positions[6] = vec2[]
(
vec2(0.0, 0.0),
vec2(0.0, 1.0),
vec2(1.0, 1.0),
vec2(1.0, 1.0),
vec2(1.0, 0.0),
vec2(0.0, 0.0)
);

struct QuadDrawInfo
{
    mat4 mpMatrix;// 64 bytes, aligned to 16 bytes
    vec4 color;// 16 bytes, aligned to 16 bytes
    vec2 uvp1;// 8 bytes, aligned to 8 bytes
    vec2 uvp2;// 8 bytes, aligned to 8 bytes
    uint textureIndex;// 4 bytes, aligned to 4 bytes
    bool useRedAsAlpha;
};

layout (location = 0) out vec2 uv;

layout (push_constant) uniform PushConstants
{
    QuadDrawInfo quad;
    uint index;
} pushConstants;

void main()
{
    gl_Position = pushConstants.quad.mpMatrix * vec4(positions[gl_VertexIndex], 1.0, 1.0);

    uv.x = mix(pushConstants.quad.uvp1.x, pushConstants.quad.uvp2.x, positions[gl_VertexIndex].x);
    uv.y = mix(pushConstants.quad.uvp1.y, pushConstants.quad.uvp2.y, positions[gl_VertexIndex].y);
}