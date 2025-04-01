#pragma once

#include <cstdint>

class DrawStats
{
public:
    void IndirectDraw(uint32_t drawCommandSize, uint32_t indexCount);
    void DrawIndexed(uint32_t indexCount);
    void Draw(uint32_t vertexCount);
    void SetParticleCount(uint32_t particleCount);
    void SetEmitterCount(uint32_t emitterCount);

    void Clear();

    uint32_t IndexCount() const { return _indexCount; }
    uint32_t DrawCalls() const { return _drawCalls; }
    uint32_t DirectDrawCommands() const { return _directDrawCommands; }
    uint32_t IndirectDrawCommands() const { return _indirectDrawCommands; }
    uint32_t GetParticleCount() const { return _particleCount; }
    uint32_t GetEmitterCount() const { return _emitterCount; }

private:
    uint32_t _indexCount;
    uint32_t _drawCalls;
    uint32_t _directDrawCommands;
    uint32_t _indirectDrawCommands;
    uint32_t _particleCount;
    uint32_t _emitterCount;
};
