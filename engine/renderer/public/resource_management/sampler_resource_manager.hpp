#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "common.hpp"
#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanContext;

class SamplerResourceManager final : public ResourceManager<Sampler>
{
public:
    SamplerResourceManager(const std::shared_ptr<VulkanContext> context);

    NON_COPYABLE(SamplerResourceManager);
    NON_MOVABLE(SamplerResourceManager);

    ResourceHandle<Sampler> Create(const SamplerCreation& creation);

private:
    std::shared_ptr<VulkanContext> _context;
};