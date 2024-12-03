#include "resource_management/sampler_resource_manager.hpp"

#include "../vulkan_helper.hpp"
#include "gpu_resources.hpp"
#include "vulkan_context.hpp"

SamplerResourceManager::SamplerResourceManager(const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
}

ResourceHandle<Sampler> SamplerResourceManager::Create(const SamplerCreation& creation)
{
    return ResourceManager::Create(Sampler { creation, _context });
}
