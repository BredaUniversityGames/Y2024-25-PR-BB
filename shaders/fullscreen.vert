#version 460

layout (location = 0) out vec2 texCoords;

void main()
{
    float x = float(((2 - gl_VertexIndex) & 1) << 2);
    float y = float(((2 - gl_VertexIndex) & 2) << 1);
    texCoords.x = x * 0.5;
    texCoords.y = y * 0.5;
    gl_Position = vec4(x - 1.0, y - 1.0, 0, 1);
}