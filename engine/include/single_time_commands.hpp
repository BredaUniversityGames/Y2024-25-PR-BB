#pragma once
#include <cstddef>
#include <class_decorations.hpp>
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <vma.hpp>

class VulkanBrain;
struct Texture;
struct TextureHandle;

class SingleTimeCommands
{
public:
    SingleTimeCommands(const VulkanBrain& brain);
    ~SingleTimeCommands();

    void Submit();
    void CreateLocalBuffer(const std::byte* vec, uint32_t count, vk::Buffer& buffer, VmaAllocation& allocation, vk::BufferUsageFlags usage, std::string_view name);
    void CopyIntoLocalBuffer(const std::byte* vec, uint32_t count, uint32_t offset, vk::Buffer buffer);

    template <typename T>
    void CreateLocalBuffer(const std::vector<T>& vec, vk::Buffer& buffer, VmaAllocation& allocation, vk::BufferUsageFlags usage, std::string_view name)
    {
        CreateLocalBuffer(reinterpret_cast<const std::byte*>(vec.data()), vec.size() * sizeof(T), buffer, allocation, usage, name);
    }

    template <typename T>
    void CopyIntoLocalBuffer(const std::vector<T>& vec, uint32_t offset, vk::Buffer buffer)
    {
        CopyIntoLocalBuffer(reinterpret_cast<const std::byte*>(vec.data()), vec.size() * sizeof(T), offset * sizeof(T), buffer);
    }

    const vk::CommandBuffer& CommandBuffer() const { return _commandBuffer; }

    NON_MOVABLE(SingleTimeCommands);
    NON_COPYABLE(SingleTimeCommands);

private:
    const VulkanBrain& _brain;
    vk::CommandBuffer _commandBuffer;
    vk::Fence _fence;
    bool _submitted { false };

    std::vector<vk::Buffer> _stagingBuffers;
    std::vector<VmaAllocation> _stagingAllocations;
};