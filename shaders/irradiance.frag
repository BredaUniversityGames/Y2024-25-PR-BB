#version 460

layout(location = 0) in vec2 texCoords;

layout(push_constant) uniform PushConstants
{
    uint index;
} face;

layout(set = 0, binding = 0) uniform sampler2D hdri;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 MapDirection(vec2 coords, uint faceIndex);
vec2 SampleSphericalMap(vec3 dir);

void main()
{
    vec3 direction = MapDirection(texCoords, face.index);
    vec4 color = texture(hdri, SampleSphericalMap(direction));

    vec3 irradiance = vec3(0.0);
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, direction));
    up = normalize(cross(direction, right));

    float sampleDelta = 0.01;
    float nrSamples = 0.0;
    for(float phi = 0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * direction;

            irradiance += texture(hdri, SampleSphericalMap(sampleVec)).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / nrSamples);

    outColor = vec4(irradiance, 1.0);
}

vec3 MapDirection(vec2 coords, uint faceIndex)
{
    vec2 uvRemapped = coords * 2.0 - 1.0;
    vec3 direction;
    if (faceIndex == 0) {  // +X face
        direction = vec3(1.0, -uvRemapped.y, -uvRemapped.x);
    } else if (faceIndex == 1) {  // -X face
        direction = vec3(-1.0, -uvRemapped.y, uvRemapped.x);
    } else if (faceIndex == 2) {  // +Y face
        direction = vec3(uvRemapped.x, 1.0, uvRemapped.y);
    } else if (faceIndex == 3) {  // -Y face
        direction = vec3(uvRemapped.x, -1.0, -uvRemapped.y);
    } else if (faceIndex == 4) {  // +Z face
        direction = vec3(uvRemapped.x, -uvRemapped.y, 1.0);
    } else if (faceIndex == 5) {  // -Z face
        direction = vec3(-uvRemapped.x, -uvRemapped.y, -1.0);
    }

    return normalize(direction);
}

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 dir)
{
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}