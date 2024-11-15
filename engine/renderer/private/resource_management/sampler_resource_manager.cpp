#include "resource_management/sampler_resource_manager.hpp"

#include "../vulkan_context.hpp"
#include "../vulkan_helper.hpp"
#include "gpu_resources.hpp"

SamplerCreateInfo& SamplerCreateInfo::setGlobalAddressMode(vk::SamplerAddressMode addressMode_)
{
    addressModeU = addressMode_;
    addressModeV = addressMode_;
    addressModeW = addressMode_;

    return *this;
}

SamplerResourceManager::SamplerResourceManager(const std::shared_ptr<VulkanContext> context)
    : _context(context)
{
}

ResourceHandle<Sampler> SamplerResourceManager::Create(const SamplerCreateInfo& creation)
{
    vk::SamplerCreateInfo createInfo {};
    if (creation.useMaxAnisotropy)
    {
        auto properties = _context->PhysicalDevice().getProperties();
        createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    }

    createInfo.addressModeU = creation.addressModeU;
    createInfo.addressModeV = creation.addressModeV;
    createInfo.addressModeW = creation.addressModeW;
    createInfo.mipmapMode = creation.mipmapMode;
    createInfo.minLod = creation.minLod;
    createInfo.maxLod = creation.maxLod;
    createInfo.compareOp = creation.compareOp;
    createInfo.compareEnable = creation.compareEnable;
    createInfo.unnormalizedCoordinates = creation.unnormalizedCoordinates;
    createInfo.mipLodBias = creation.mipLodBias;
    createInfo.borderColor = creation.borderColor;
    createInfo.minFilter = creation.minFilter;
    createInfo.magFilter = creation.magFilter;

    ResourceHandle<Sampler> samplerHandle = ResourceManager::Create(static_cast<Sampler>(_context->Device().createSampler(createInfo)));

    const Sampler* sampler = Access(samplerHandle);

    util::NameObject(*sampler, creation.name, _context);

    return samplerHandle;
}

void SamplerResourceManager::Destroy(ResourceHandle<Sampler> handle)
{
    const Sampler* sampler { Access(handle) };
    if (sampler == nullptr)
    {
        return;
    }

    _context->Device().destroy(*sampler);

    ResourceManager::Destroy(handle);
}
