#include "resource_management/buffer_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

BufferResourceManager::BufferResourceManager(const VulkanContext& brain)
    : _brain(brain)
{
}

ResourceHandle<Buffer> BufferResourceManager::Create(const BufferCreation& creation)
{
    Buffer bufferResource {};

    util::CreateBuffer(_brain,
        creation.size,
        creation.usage,
        bufferResource.buffer,
        creation.isMappable,
        bufferResource.allocation,
        creation.memoryUsage,
        creation.name);

    bufferResource.size = creation.size;
    bufferResource.usage = creation.usage;
    bufferResource.name = creation.name;

    if (creation.isMappable)
    {
        util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, bufferResource.allocation, &bufferResource.mappedPtr),
            "Failed mapping memory for buffer: " + creation.name);
    }

    return ResourceManager::Create(bufferResource);
}

void BufferResourceManager::Destroy(ResourceHandle<Buffer> handle)
{
    if (IsValid(handle))
    {
        const Buffer* buffer = Access(handle);

        if (buffer->mappedPtr)
        {
            vmaUnmapMemory(_brain.vmaAllocator, buffer->allocation);
        }

        vmaDestroyBuffer(_brain.vmaAllocator, buffer->buffer, buffer->allocation);

        ResourceManager::Destroy(handle);
    }
}