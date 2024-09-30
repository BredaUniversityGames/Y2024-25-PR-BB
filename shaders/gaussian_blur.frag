#version 460

layout(push_constant) uniform PushConstants
{
    uint vertical;
} passType;

layout(binding = 0) uniform sampler2D source;

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

const float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

void main()
{
    vec2 texOffset = 1.0 / textureSize(source, 0);
    vec3 result = texture(source, texCoords).rgb * weight[0];

    if (passType.vertical == 1)
    {
        for (int i = 1; i < 5; ++i)
        {
            result += texture(source, texCoords + vec2(0.0, texOffset.y * i)).rgb * weight[i];
            result += texture(source, texCoords - vec2(0.0, texOffset.y * i)).rgb * weight[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            result += texture(source, texCoords + vec2(texOffset.x * i, 0.0)).rgb * weight[i];
            result += texture(source, texCoords - vec2(texOffset.x * i, 0.0)).rgb * weight[i];
        }
    }

    outColor = vec4(result, 1.0);
}