#pragma once

struct alignas(16) QuadDrawInfo
{
    glm::mat4 modelMatrix;
    glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
    glm::vec2 uvp1 = { 0.f, 0.f };
    glm::vec2 uvp2 = { 1.f, 1.f };
    uint32_t textureIndex;
    uint32_t useRedAsAlpha = false;
};
