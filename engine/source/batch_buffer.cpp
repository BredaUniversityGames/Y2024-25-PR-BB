#include "batch_buffer.hpp"
#include "vulkan_helper.hpp"
#include "single_time_commands.hpp"

BatchBuffer::BatchBuffer(const VulkanBrain& brain, uint32_t vertexBufferSize, uint32_t indexBufferSize)
    : _brain(brain)
    , _vertexBufferSize(vertexBufferSize)
    , _indexBufferSize(indexBufferSize)
    , _indexType(vk::IndexType::eUint32)
    , _topology(vk::PrimitiveTopology::eTriangleList)
{
    util::CreateBuffer(
        _brain,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        _vertexBuffer,
        false,
        _vertexBufferAllocation,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "Unified vertex buffer");

    util::CreateBuffer(
        _brain,
        _indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        _indexBuffer,
        false,
        _indexBufferAllocation,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "Unified index buffer");
}

BatchBuffer::~BatchBuffer()
{

    vmaDestroyBuffer(_brain.vmaAllocator, _vertexBuffer, _vertexBufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _indexBuffer, _indexBufferAllocation);
}

uint32_t BatchBuffer::AppendVertices(const std::vector<Vertex>& vertices, SingleTimeCommands& commandBuffer)
{
    assert((_vertexOffset + vertices.size()) * sizeof(Vertex) < _vertexBufferSize);
    uint32_t originalOffset = _vertexOffset;

    commandBuffer.CopyIntoLocalBuffer(vertices, _vertexOffset, _vertexBuffer);

    _vertexOffset += vertices.size();

    return originalOffset;
}

uint32_t BatchBuffer::AppendIndices(const std::vector<uint32_t>& indices, SingleTimeCommands& commandBuffer)
{
    assert((_indexOffset + indices.size()) * sizeof(uint32_t) < _indexBufferSize);
    uint32_t originalOffset = _indexOffset;

    commandBuffer.CopyIntoLocalBuffer(indices, _indexOffset, _indexBuffer);

    _indexOffset += indices.size();

    return originalOffset;
}
