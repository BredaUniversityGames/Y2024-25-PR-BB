#include "resource_management/sampler_resource_manager.hpp"

#include "../vulkan_helper.hpp"
#include "gpu_resources.hpp"
#include "vulkan_context.hpp"

template <>
std::weak_ptr<ResourceManager<Sampler>> ResourceHandle<Sampler>::manager = {};

SamplerResourceManager::SamplerResourceManager(const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
}

ResourceHandle<Sampler> SamplerResourceManager::Create(const SamplerCreation& creation)
{
    return ResourceManager::Create(Sampler { creation, _context });
}
