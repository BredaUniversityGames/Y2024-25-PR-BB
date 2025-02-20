#include "memory/vma_helper.hpp"

#include <tracy/TracyVulkan.hpp>
#include <utility>

vma::BufferAllocation::~BufferAllocation()
{
    if (allocation)
        vma::DestroyBuffer(manager, buffer, allocation);
}

vma::BufferAllocation::BufferAllocation(BufferAllocation&& o) noexcept
{
    std::swap(o.manager, manager);
    std::swap(o.buffer, buffer);
    std::swap(o.allocation, allocation);
}

vma::BufferAllocation& vma::BufferAllocation::operator=(BufferAllocation&& o) noexcept
{
    if (allocation)
        vma::DestroyBuffer(manager, buffer, allocation);
    allocation = nullptr;

    std::swap(o.manager, manager);
    std::swap(o.buffer, buffer);
    std::swap(o.allocation, allocation);

    return *this;
}

vma::ImageAllocation::~ImageAllocation()
{
    if (allocation)
        vma::DestroyImage(manager, image, allocation);
}

vma::ImageAllocation::ImageAllocation(ImageAllocation&& o) noexcept
{
    std::swap(o.manager, manager);
    std::swap(o.image, image);
    std::swap(o.allocation, allocation);
}

vma::ImageAllocation& vma::ImageAllocation::operator=(ImageAllocation&& o) noexcept
{
    if (allocation)
        vma::DestroyImage(manager, image, allocation);
    allocation = nullptr;

    std::swap(o.manager, manager);
    std::swap(o.image, image);
    std::swap(o.allocation, allocation);

    return *this;
}

vma::BufferAllocation vma::AllocateBuffer(VmaAllocator allocator, const VkBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, const char* allocName)
{
    VmaAllocation outAllocation;
    VkBuffer outBuffer;

    if (CreateBuffer(allocator, pBufferCreateInfo, pAllocationCreateInfo, &outBuffer, &outAllocation, nullptr) == VK_SUCCESS)
    {
        BufferAllocation out;
        out.manager = allocator;
        out.buffer = outBuffer;
        out.allocation = outAllocation;

        if (allocName)
            vmaSetAllocationName(allocator, outAllocation, allocName);

        return out;
    }

    return {};
}

vma::ImageAllocation vma::AllocateImage(VmaAllocator allocator, const VkImageCreateInfo* pImageCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, const char* allocName)
{
    VmaAllocation outAllocation;
    VkImage outImage;

    if (CreateImage(allocator, pImageCreateInfo, pAllocationCreateInfo, &outImage, &outAllocation, nullptr) == VK_SUCCESS)
    {
        ImageAllocation out;
        out.manager = allocator;
        out.image = outImage;
        out.allocation = outAllocation;

        if (allocName)
            vmaSetAllocationName(allocator, outAllocation, allocName);

        return out;
    }

    return {};
}

VkResult vma::CreateBuffer(VmaAllocator allocator, const VkBufferCreateInfo* pBufferCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, VkBuffer* pBuffer, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo)
{
#ifdef TRACY_ENABLE
    VmaAllocationInfo allocInfo {};
    if (pAllocationInfo == nullptr)
    {
        pAllocationInfo = &allocInfo;
    }
#endif

    auto result = ::vmaCreateBuffer(allocator, pBufferCreateInfo, pAllocationCreateInfo, pBuffer, pAllocation, pAllocationInfo);

#ifdef TRACY_ENABLE
    if (result == VK_SUCCESS)
    {
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage");
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage (Buffer)");
    }
#endif

    return result;
}

void vma::DestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
#ifdef TRACY_ENABLE
    TracyFreeN(allocation, "GPU Memory usage");
    TracyFreeN(allocation, "GPU Memory usage (Buffer)");
#endif

    ::vmaDestroyBuffer(allocator, buffer, allocation);
}

VkResult vma::CreateImage(VmaAllocator allocator, const VkImageCreateInfo* pImageCreateInfo, const VmaAllocationCreateInfo* pAllocationCreateInfo, VkImage* pImage, VmaAllocation* pAllocation, VmaAllocationInfo* pAllocationInfo)
{
#ifdef TRACY_ENABLE
    VmaAllocationInfo allocInfo;
    if (pAllocationInfo == nullptr)
    {
        pAllocationInfo = &allocInfo;
    }
#endif

    auto result = ::vmaCreateImage(allocator, pImageCreateInfo, pAllocationCreateInfo, pImage, pAllocation, pAllocationInfo);

#ifdef TRACY_ENABLE
    if (result == VK_SUCCESS)
    {
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage");
        TracyAllocN(*pAllocation, pAllocationInfo->size, "GPU Memory usage (Image)");
    }
#endif

    return result;
}

void vma::DestroyImage(VmaAllocator allocator, VkImage image, VmaAllocation allocation)
{
#ifdef TRACY_ENABLE
    TracyFreeN(allocation, "GPU Memory usage");
    TracyFreeN(allocation, "GPU Memory usage (Image)");
#endif

    ::vmaDestroyImage(allocator, image, allocation);
}