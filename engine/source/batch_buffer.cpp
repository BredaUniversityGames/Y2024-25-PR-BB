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

    InitializeDescriptorSets();
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

void BatchBuffer::InitializeDescriptorSets()
{
    vk::DescriptorSetLayoutBinding layoutBinding {};
    layoutBinding.binding = 0;
    layoutBinding.stageFlags = vk::ShaderStageFlagBits::eCompute;
    layoutBinding.descriptorType = vk::DescriptorType::eStorageBuffer;

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {};
    descriptorSetLayoutCreateInfo.bindingCount = 1;
    descriptorSetLayoutCreateInfo.pBindings = &layoutBinding;

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, &_drawBufferDescriptorSetLayout), "Failed creating descriptor set layout!");

    std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts {};
    std::for_each(layouts.begin(), layouts.end(), [this](auto& l)
        { l = _drawBufferDescriptorSetLayout; });
    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocateInfo.pSetLayouts = layouts.data();

    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
        "Failed allocating descriptor sets!");
    for (size_t i = 0; i < descriptorSets.size(); ++i)
    {
        _drawBufferDescriptorSets[i] = descriptorSets[i];
        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = _indirectDrawBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = vk::WholeSize;

        vk::WriteDescriptorSet bufferWrite {};
        bufferWrite.dstSet = _drawBufferDescriptorSets[i];
        bufferWrite.dstBinding = 0;
        bufferWrite.dstArrayElement = 0;
        bufferWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
        bufferWrite.descriptorCount = 1;
        bufferWrite.pBufferInfo = &bufferInfo;

        _brain.device.updateDescriptorSets(1, &bufferWrite, 0, nullptr);
    }
}
