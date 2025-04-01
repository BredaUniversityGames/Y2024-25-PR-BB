#include "draw_stats.hpp"

#include <cstdint>

void DrawStats::IndirectDraw(uint32_t drawCommandSize, uint32_t indexCount)
{
    _drawCalls++;
    _indirectDrawCommands += drawCommandSize;
    _indexCount += indexCount;
}

void DrawStats::DrawIndexed(uint32_t indexCount)
{
    _directDrawCommands++;
    _drawCalls++;
    _indexCount += indexCount;
}

void DrawStats::Draw(uint32_t vertexCount)
{
    _drawCalls++;
    _indexCount += vertexCount;
}

void DrawStats::SetParticleCount(uint32_t particleCount)
{
    _particleCount = particleCount;
}

void DrawStats::SetEmitterCount(uint32_t emitterCount)
{
    _emitterCount = emitterCount;
}

void DrawStats::Clear()
{
    _drawCalls = 0;
    _indexCount = 0;
    _directDrawCommands = 0;
    _indirectDrawCommands = 0;
    _particleCount = 0;
    _emitterCount = 0;
}
