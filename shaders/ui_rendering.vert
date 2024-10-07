#version 450

layout(location = 0) out vec2 uv;

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0,1.0),
    vec2(1.0,0.0),
    vec2(0.0,0.0)
);

vec2 uvs[6] = vec2[](
    vec2(0.0,0.0),
    vec2(0.0,1.0),
    vec2(1.0,1.0),
    vec2(1.0,1.0),
    vec2(1.0,0.0),
    vec2(0.0,0.0)
);


layout( push_constant ) uniform PushConstants
{	
	mat4 MVP;
    	uint TextureIndex;
} pushConstants;


void main() {
    gl_Position = pushConstants.render_matrix* vec4(positions[gl_VertexIndex], 0.0, 1.0);
    uv = uvs[gl_VertexIndex];
}