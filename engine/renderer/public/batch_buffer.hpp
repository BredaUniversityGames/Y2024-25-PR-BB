#pragma once
#include "common.hpp"
#include "mesh.hpp"

class SingleTimeCommands;
class VulkanBrain;

constexpr uint32_t MAX_MESHES = 2048;

class BatchBuffer
{
public:
    BatchBuffer(const VulkanBrain& brain, uint32_t vertexBufferSize, uint32_t indexBufferSize);
    ~BatchBuffer();
    NON_MOVABLE(BatchBuffer);
    NON_COPYABLE(BatchBuffer);

    ResourceHandle<Buffer> VertexBuffer() const { return _vertexBuffer; }
    ResourceHandle<Buffer> IndexBuffer() const { return _indexBuffer; };
    vk::IndexType IndexType() const { return _indexType; }
    vk::PrimitiveTopology Topology() const { return _topology; }

    uint32_t VertexBufferSize() const { return _vertexBufferSize; }
    uint32_t IndexBufferSize() const { return _indexBufferSize; }

    ResourceHandle<Buffer> IndirectDrawBuffer(uint32_t frameIndex) const { return _indirectDrawBuffers[frameIndex]; }

    uint32_t AppendVertices(const std::vector<Vertex>& vertices, SingleTimeCommands& commandBuffer);
    uint32_t AppendIndices(const std::vector<uint32_t>& indices, SingleTimeCommands& commandBuffer);

private:
    const VulkanBrain& _brain;

    uint32_t _vertexBufferSize;
    uint32_t _indexBufferSize;
    vk::IndexType _indexType;
    vk::PrimitiveTopology _topology;

    ResourceHandle<Buffer> _vertexBuffer;
    ResourceHandle<Buffer> _indexBuffer;
    std::array<ResourceHandle<Buffer>, MAX_FRAMES_IN_FLIGHT> _indirectDrawBuffers;

    uint32_t _vertexOffset { 0 };
    uint32_t _indexOffset { 0 };
};