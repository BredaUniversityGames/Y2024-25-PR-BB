#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set =0, binding = 0) uniform usampler2D displayTexture;


void main()
{
    outColor = vec4(  texture(displayTexture,uv).r);
}   