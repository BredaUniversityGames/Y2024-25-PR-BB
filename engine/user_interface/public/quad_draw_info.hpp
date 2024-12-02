#pragma once
#include <glm/glm.hpp>
#include <stdint.h>

struct alignas(16) QuadDrawInfo
{
    glm::mat4 modelMatrix; 
    glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
    glm::vec2 uvMin = { 0.f, 0.f };
    glm::vec2 uvMax = { 1.f, 1.f };
    uint32_t textureIndex;
    uint32_t useRedAsAlpha = false;
};
