#pragma once

struct alignas(16) QuadDrawInfo
{
    
    // todo:: remove alignas
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec4 color = { 1.f, 1.f, 1.f, 1.f };
    alignas(8) glm::vec2 uvp1 = { 0.f, 0.f };
    alignas(8) glm::vec2 uvp2 = { 1.f, 1.f };
    alignas(4) uint32_t textureIndex;
    alignas(4) uint32_t useRedAsAlpha = false;
};