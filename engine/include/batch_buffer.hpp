#pragma once
#include "class_decorations.hpp"
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

    vk::Buffer VertexBuffer() const { return _vertexBuffer; }
    vk::Buffer IndexBuffer() const { return _indexBuffer; }
    vk::IndexType IndexType() const { return _indexType; }
    vk::PrimitiveTopology Topology() const { return _topology; }

    uint32_t VertexBufferSize() const { return _vertexBufferSize; }
    uint32_t IndexBufferSize() const { return _indexBufferSize; }

    vk::Buffer IndirectDrawBuffer(uint32_t frameIndex) const { return _indirectDrawBuffers[frameIndex]; }
    vk::DescriptorSetLayout DrawBufferLayout() const { return _drawBufferDescriptorSetLayout; }
    vk::DescriptorSet DrawBufferDescriptorSet(uint32_t frameIndex) const { return _drawBufferDescriptorSets[frameIndex]; }

    vk::Buffer IndirectCountBuffer(uint32_t frameIndex) const { return _indirectDrawBuffers[frameIndex]; }
    uint32_t IndirectCountOffset() const { return MAX_MESHES * sizeof(vk::DrawIndexedIndirectCommand); }

    uint32_t AppendVertices(const std::vector<Vertex>& vertices, SingleTimeCommands& commandBuffer);
    uint32_t AppendIndices(const std::vector<uint32_t>& indices, SingleTimeCommands& commandBuffer);

    uint32_t DrawCount() const { return _drawCount; };

    void WriteDraws(const std::vector<vk::DrawIndexedIndirectCommand>& commands, uint32_t frameIndex) const;

private:
    const VulkanBrain& _brain;

    uint32_t _vertexBufferSize;
    uint32_t _indexBufferSize;
    vk::IndexType _indexType;
    vk::PrimitiveTopology _topology;
    vk::Buffer _vertexBuffer;
    vk::Buffer _indexBuffer;
    VmaAllocation _vertexBufferAllocation;
    VmaAllocation _indexBufferAllocation;

    std::array<vk::Buffer, MAX_FRAMES_IN_FLIGHT> _indirectDrawBuffers;
    vk::DescriptorSetLayout _drawBufferDescriptorSetLayout;
    std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> _drawBufferDescriptorSets;
    std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> _indirectDrawBufferAllocations;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> _indirectDrawBufferPtr;

    uint32_t _vertexOffset { 0 };
    uint32_t _indexOffset { 0 };

    mutable uint32_t _drawCount { 0 };

    void InitializeDescriptorSets();
};