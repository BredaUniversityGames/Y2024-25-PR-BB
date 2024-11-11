#pragma once

#include <cstddef>

class DrawStats
{
public:
    void IndirectDraw(uint32_t drawCommandSize, uint32_t indexCount);
    void Draw(uint32_t vertexCount);

    void Clear();

    uint32_t IndexCount() const { return _indexCount; }
    uint32_t DrawCalls() const { return _drawCalls; }
    uint32_t IndirectDrawCommands() const { return _indirectDrawCommands; }

private:
    uint32_t _indexCount;
    uint32_t _drawCalls;
    uint32_t _indirectDrawCommands;
};