#include "resources/sampler.hpp"
#include "vulkan_helper.hpp"

SamplerCreation& SamplerCreation::SetGlobalAddressMode(vk::SamplerAddressMode addressMode)
{
    addressModeU = addressMode;
    addressModeV = addressMode;
    addressModeW = addressMode;

    return *this;
}

Sampler::Sampler(const SamplerCreation& creation, const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
    vk::StructureChain<vk::SamplerCreateInfo, vk::SamplerReductionModeCreateInfo> structureChain {};
    vk::SamplerCreateInfo& createInfo { structureChain.get<vk::SamplerCreateInfo>() };
    if (creation.useMaxAnisotropy)
    {
        auto properties = _context->PhysicalDevice().getProperties();
        createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    }

    vk::SamplerReductionModeCreateInfo& reductionModeCreateInfo { structureChain.get<vk::SamplerReductionModeCreateInfo>() };
    reductionModeCreateInfo.reductionMode = creation.reductionMode;

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

    sampler = _context->Device().createSampler(createInfo);

    util::NameObject(sampler, creation.name, _context);
}

Sampler::~Sampler()
{
    if (!_context)
    {
        return;
    }

    _context->Device().destroy(sampler);
}

Sampler::Sampler(Sampler&& other) noexcept
    : sampler(other.sampler)
    , _context(other._context)
{
    other.sampler = nullptr;
    other._context = nullptr;
}

Sampler& Sampler::operator=(Sampler&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    sampler = other.sampler;
    _context = other._context;

    other.sampler = nullptr;
    other._context = nullptr;

    return *this;
}