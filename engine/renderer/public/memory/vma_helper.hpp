#pragma once
#include "common.hpp"
#include <vk_mem_alloc.h>

namespace vma
{

struct BufferAllocation
{
    BufferAllocation() = default;
    ~BufferAllocation();

    NON_COPYABLE(BufferAllocation);
    BufferAllocation(BufferAllocation&& o) noexcept;
    BufferAllocation& operator=(BufferAllocation&& o) noexcept;

    operator bool() const
    {
        return allocation != nullptr;
    }

    VmaAllocator manager {};
    VmaAllocation allocation {};
    VkBuffer buffer {};
};

struct ImageAllocation
{
    ImageAllocation() = default;
    ~ImageAllocation();

    NON_COPYABLE(ImageAllocation);
    ImageAllocation(ImageAllocation&& o) noexcept;
    ImageAllocation& operator=(ImageAllocation&& o) noexcept;

    operator bool() const
    {
        return allocation != nullptr;
    }

    VmaAllocator manager {};
    VmaAllocation allocation {};
    VkImage image {};
};

BufferAllocation AllocateBuffer(VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    const char* allocName = nullptr);

ImageAllocation AllocateImage(VmaAllocator allocator,
    const VkImageCreateInfo* pImageCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    const char* allocName = nullptr);

VkResult CreateBuffer(VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

VkResult CreateImage(VmaAllocator allocator,
    const VkImageCreateInfo* pImageCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkImage* pImage,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

void DestroyImage(
    VmaAllocator allocator,
    VkImage image,
    VmaAllocation allocation);

void DestroyBuffer(
    VmaAllocator allocator,
    VkBuffer buffer,
    VmaAllocation allocation);

}