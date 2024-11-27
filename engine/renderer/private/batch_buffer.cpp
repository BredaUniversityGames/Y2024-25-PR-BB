#include "batch_buffer.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

BatchBuffer::BatchBuffer(const std::shared_ptr<GraphicsContext>& context, uint32_t vertexBufferSize, uint32_t indexBufferSize)
    : _context(context)
    , _vertexBufferSize(vertexBufferSize)
    , _indexBufferSize(indexBufferSize)
    , _indexType(vk::IndexType::eUint32)
    , _topology(vk::PrimitiveTopology::eTriangleList)
{
    auto resources { _context->Resources() };

    BufferCreation vertexBufferCreation {};
    vertexBufferCreation.SetSize(vertexBufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Unified vertex buffer");

    _vertexBuffer = resources->BufferResourceManager().Create(vertexBufferCreation);

    BufferCreation indexBufferCreation {};
    indexBufferCreation.SetSize(indexBufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Unified index buffer");

    _indexBuffer = resources->BufferResourceManager().Create(indexBufferCreation);
}

BatchBuffer::~BatchBuffer()
{
    auto resources { _context->Resources() };
}

uint32_t BatchBuffer::AppendIndices(const std::vector<uint32_t>& indices, SingleTimeCommands& commandBuffer)
{
    auto resources { _context->Resources() };

    assert((_indexOffset + indices.size()) * sizeof(uint32_t) < _indexBufferSize);
    uint32_t originalOffset = _indexOffset;

    const Buffer* buffer = resources->BufferResourceManager().Access(_indexBuffer);
    commandBuffer.CopyIntoLocalBuffer(indices, _indexOffset, buffer->buffer);

    _indexOffset += indices.size();

    return originalOffset;
}
