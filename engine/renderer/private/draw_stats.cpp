#include "draw_stats.hpp"

void DrawStats::IndirectDraw(uint32_t drawCommandSize, uint32_t indexCount)
{
    _drawCalls++;
    _indirectDrawCommands += drawCommandSize;
    _indexCount += indexCount;
}

void DrawStats::Draw(uint32_t vertexCount)
{
    _drawCalls++;
    _indexCount += vertexCount;
}

void DrawStats::Clear()
{
    _drawCalls = 0;
    _indexCount = 0;
    _indirectDrawCommands = 0;
}
