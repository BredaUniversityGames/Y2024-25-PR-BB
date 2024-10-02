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

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        util::CreateBuffer(
            _brain,
            sizeof(vk::DrawIndexedIndirectCommand) * MAX_MESHES,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer,
            _indirectDrawBuffers[i],
            true,
            _indirectDrawBufferAllocations[i],
            VMA_MEMORY_USAGE_AUTO,
            "Indirect draw buffer");

        vmaMapMemory(_brain.vmaAllocator, _indirectDrawBufferAllocations[i], &_indirectDrawBufferPtr[i]);
    }
}

BatchBuffer::~BatchBuffer()
{
    vmaDestroyBuffer(_brain.vmaAllocator, _vertexBuffer, _vertexBufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _indexBuffer, _indexBufferAllocation);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vmaUnmapMemory(_brain.vmaAllocator, _indirectDrawBufferAllocations[i]);
        vmaDestroyBuffer(_brain.vmaAllocator, _indirectDrawBuffers[i], _indirectDrawBufferAllocations[i]);
    }
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

void BatchBuffer::WriteDraws(const std::vector<vk::DrawIndexedIndirectCommand>& commands, uint32_t frameIndex) const
{
    assert(commands.size() < MAX_MESHES && "Too many draw commands");

    _drawCount = commands.size();
    std::memcpy(_indirectDrawBufferPtr[frameIndex], commands.data(), commands.size() * sizeof(vk::DrawIndexedIndirectCommand));
}
