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
    explicit SamplerResourceManager(const std::shared_ptr<VulkanContext> context);
    ResourceHandle<Sampler> Create(const SamplerCreation& creation);

    ResourceHandle<Sampler> GetDefaultSamplerHandle() const noexcept { return _defaultSampler; };

private:
    std::shared_ptr<VulkanContext> _context;

    ResourceHandle<Sampler> _defaultSampler;
};