#pragma once

#include "vulkan_context.hpp"

#include <memory>
#include <vk_mem_alloc.h>

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

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    NON_COPYABLE(Buffer);

    vk::Buffer buffer {};
    VmaAllocation allocation {};
    void* mappedPtr = nullptr;
    vk::DeviceSize size {};
    vk::BufferUsageFlags usage {};
    std::string name {};

private:
    std::shared_ptr<VulkanContext> _context;
};
