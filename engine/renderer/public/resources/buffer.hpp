#pragma once
#include "common.hpp"
#include "memory/vma_helper.hpp"
#include "vulkan_include.hpp"

class VulkanContext;

struct BufferCreation
{
    vk::DeviceSize size {};
    vk::BufferUsageFlags usage {};
    bool isMappable = true;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
    std::string name {};

    BufferCreation& SetSize(vk::DeviceSize size);
    BufferCreation& SetUsageFlags(vk::BufferUsageFlags usage);
    BufferCreation& SetIsMappable(bool isMappable);
    BufferCreation& SetMemoryUsage(VmaMemoryUsage memoryUsage);
    BufferCreation& SetName(std::string_view name);
};

struct Buffer
{
    Buffer(const BufferCreation& creation, const std::shared_ptr<VulkanContext>& context);
    ~Buffer();

    vk::Buffer Get() const { return _buffer.buffer; }

    NON_COPYABLE(Buffer);
    DEFAULT_MOVABLE(Buffer);

    void* mappedPtr = nullptr;
    vk::DeviceSize size {};
    vk::BufferUsageFlags usage {};
    std::string name {};

private:
    vma::BufferAllocation _buffer {};
    std::shared_ptr<VulkanContext> _context;
};