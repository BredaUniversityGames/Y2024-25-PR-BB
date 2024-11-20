#include "resource_management/buffer_resource_manager.hpp"

#include "vulkan_context.hpp"

template <>
std::weak_ptr<ResourceManager<Buffer>> ResourceHandle<Buffer>::manager = {};

BufferResourceManager::BufferResourceManager(const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
}

ResourceHandle<Buffer> BufferResourceManager::Create(const BufferCreation& creation)
{
    return ResourceManager::Create(Buffer { creation, _context });
}