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
    explicit SamplerResourceManager(const std::shared_ptr<VulkanContext>& context);
    ResourceHandle<Sampler> Create(const SamplerCreation& creation);

    // create default fallback sampler
    void CreateDefaultSampler()
    {
        SamplerCreation info;
        info.name = "Default sampler";
        info.minLod = 0;
        info.maxLod = 32;
        _defaultSampler = Create(info);
    }

    ResourceHandle<Sampler> GetDefaultSamplerHandle() const noexcept { return _defaultSampler; };

private:
    std::shared_ptr<VulkanContext> _context;

    ResourceHandle<Sampler> _defaultSampler;
};