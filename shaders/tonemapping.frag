#version 460

layout(binding = 0) uniform sampler2D hdrTarget;
layout(binding = 1) uniform sampler2D bloomTarget;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

const float exposure = 0.4;

void main()
{
    vec3 hdrColor = texture(hdrTarget, texCoords).rgb;

    float bloomStrenght = 0.8;
    vec3 bloomColor = texture(bloomTarget, texCoords).rgb;
    hdrColor += bloomColor * bloomStrenght;

    // Reinhardt
    //vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    // Aces
    //vec3 mapped = aces(hdrColor);

    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / 2.2));

    outColor = vec4(mapped, 1.0);
}