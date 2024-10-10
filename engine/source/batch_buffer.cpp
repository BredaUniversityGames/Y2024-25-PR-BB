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
    BufferCreation vertexBufferCreation {};
    vertexBufferCreation.SetSize(vertexBufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Unified vertex buffer");

    _vertexBuffer = _brain.GetBufferResourceManager().Create(vertexBufferCreation);

    BufferCreation indexBufferCreation {};
    indexBufferCreation.SetSize(indexBufferSize)
        .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer)
        .SetIsMappable(false)
        .SetMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
        .SetName("Unified index buffer");

    _indexBuffer = _brain.GetBufferResourceManager().Create(indexBufferCreation);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        std::string name = "[] Indirect draw buffer";

        // Inserts i in the middle of []
        name.insert(1, 1, static_cast<char>(i + '0'));

        BufferCreation creation {};
        creation.SetSize(sizeof(vk::DrawIndexedIndirectCommand) * MAX_MESHES)
            .SetUsageFlags(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer)
            .SetMemoryUsage(VMA_MEMORY_USAGE_AUTO)
            .SetName(name);

        _indirectDrawBuffers[i] = _brain.GetBufferResourceManager().Create(creation);
    }
}

BatchBuffer::~BatchBuffer()
{
    _brain.GetBufferResourceManager().Destroy(_vertexBuffer);
    _brain.GetBufferResourceManager().Destroy(_indexBuffer);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        _brain.GetBufferResourceManager().Destroy(_indirectDrawBuffers[i]);
    }
}

uint32_t BatchBuffer::AppendVertices(const std::vector<Vertex>& vertices, SingleTimeCommands& commandBuffer)
{
    assert((_vertexOffset + vertices.size()) * sizeof(Vertex) < _vertexBufferSize);
    uint32_t originalOffset = _vertexOffset;

    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_vertexBuffer);
    commandBuffer.CopyIntoLocalBuffer(vertices, _vertexOffset, buffer->buffer);

    _vertexOffset += vertices.size();

    return originalOffset;
}

uint32_t BatchBuffer::AppendIndices(const std::vector<uint32_t>& indices, SingleTimeCommands& commandBuffer)
{
    assert((_indexOffset + indices.size()) * sizeof(uint32_t) < _indexBufferSize);
    uint32_t originalOffset = _indexOffset;

    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_indexBuffer);
    commandBuffer.CopyIntoLocalBuffer(indices, _indexOffset, buffer->buffer);

    _indexOffset += indices.size();

    return originalOffset;
}

void BatchBuffer::WriteDraws(const std::vector<vk::DrawIndexedIndirectCommand>& commands, uint32_t frameIndex) const
{
    assert(commands.size() < MAX_MESHES && "Too many draw commands");

    _drawCount = commands.size();
    const Buffer* buffer = _brain.GetBufferResourceManager().Access(_indirectDrawBuffers[frameIndex]);
    std::memcpy(buffer->mappedPtr, commands.data(), commands.size() * sizeof(vk::DrawIndexedIndirectCommand));
}
