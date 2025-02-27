#include "resources/buffer.hpp"

#include <corecrt_io.h>

BufferCreation& BufferCreation::SetSize(vk::DeviceSize size)
{
    this->size = size;
    return *this;
}

BufferCreation& BufferCreation::SetUsageFlags(vk::BufferUsageFlags usage)
{
    this->usage = usage;
    return *this;
}

BufferCreation& BufferCreation::SetIsMappable(bool isMappable)
{
    this->isMappable = isMappable;
    return *this;
}

BufferCreation& BufferCreation::SetMemoryUsage(VmaMemoryUsage memoryUsage)
{
    this->memoryUsage = memoryUsage;
    return *this;
}

BufferCreation& BufferCreation::SetName(std::string_view name)
{
    this->name = name;
    return *this;
}

Buffer::Buffer(const BufferCreation& creation, const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
    vk::BufferCreateInfo bufferInfo {};

    bufferInfo.size = creation.size;
    bufferInfo.usage = creation.usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;
    bufferInfo.queueFamilyIndexCount = 1;
    bufferInfo.pQueueFamilyIndices = &context->QueueFamilies().graphicsFamily.value();

    VmaAllocationCreateInfo allocationInfo {};
    allocationInfo.usage = creation.memoryUsage;

    if (creation.isMappable)
        allocationInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    _buffer = vma::AllocateBuffer(
        context->MemoryAllocator(),
        &static_cast<VkBufferCreateInfo&>(bufferInfo),
        &allocationInfo, creation.name.c_str());

    if (creation.isMappable)
    {
        util::VK_ASSERT(vmaMapMemory(_context->MemoryAllocator(), _buffer.allocation, &mappedPtr),
            "Failed mapping memory for buffer: " + creation.name);
    }

    size = creation.size;
    usage = creation.usage;
    name = creation.name;
}

Buffer::~Buffer()
{
    if (mappedPtr && _buffer.allocation)
    {
        vmaUnmapMemory(_buffer.manager, _buffer.allocation);
    }
}
