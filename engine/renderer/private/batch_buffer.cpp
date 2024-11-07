#include "batch_buffer.hpp"

#include "single_time_commands.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

BatchBuffer::BatchBuffer(const std::shared_ptr<VulkanContext>& context, uint32_t vertexBufferSize, uint32_t indexBufferSize)
    : _context(context)
    , _vertexBufferSize(vertexBufferSize)
    , _indexBufferSize(indexBufferSize)
    , _indexType(vk::IndexType::eUint32)
    , _topology(vk::PrimitiveTopology::eTriangleList)
{
    BufferCreation vertexBufferCreation {};
    vertexBufferCreation.SetSize(vertexBufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Unified vertex buffer");

    _vertexBuffer = _context->GetBufferResourceManager().Create(vertexBufferCreation);

    BufferCreation indexBufferCreation {};
    indexBufferCreation.SetSize(indexBufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Unified index buffer");

    _indexBuffer = _context->GetBufferResourceManager().Create(indexBufferCreation);
}

BatchBuffer::~BatchBuffer()
{
    _context->GetBufferResourceManager().Destroy(_vertexBuffer);
    _context->GetBufferResourceManager().Destroy(_indexBuffer);
}

uint32_t BatchBuffer::AppendVertices(const std::vector<Vertex>& vertices, SingleTimeCommands& commandBuffer)
{
    assert((_vertexOffset + vertices.size()) * sizeof(Vertex) < _vertexBufferSize);
    uint32_t originalOffset = _vertexOffset;

    const Buffer* buffer = _context->GetBufferResourceManager().Access(_vertexBuffer);
    commandBuffer.CopyIntoLocalBuffer(vertices, _vertexOffset, buffer->buffer);

    _vertexOffset += vertices.size();

    return originalOffset;
}

uint32_t BatchBuffer::AppendIndices(const std::vector<uint32_t>& indices, SingleTimeCommands& commandBuffer)
{
    assert((_indexOffset + indices.size()) * sizeof(uint32_t) < _indexBufferSize);
    uint32_t originalOffset = _indexOffset;

    const Buffer* buffer = _context->GetBufferResourceManager().Access(_indexBuffer);
    commandBuffer.CopyIntoLocalBuffer(indices, _indexOffset, buffer->buffer);

    _indexOffset += indices.size();

    return originalOffset;
}
