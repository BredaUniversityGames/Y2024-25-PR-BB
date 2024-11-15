#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "common.hpp"
#include "resource_manager.hpp"

class VulkanContext;
struct Sampler;

struct SamplerCreateInfo
{
    SamplerCreateInfo& setGlobalAddressMode(vk::SamplerAddressMode addressMode_);

    std::string name {};
    vk::SamplerAddressMode addressModeU { vk::SamplerAddressMode::eRepeat };
    vk::SamplerAddressMode addressModeW { vk::SamplerAddressMode::eRepeat };
    vk::SamplerAddressMode addressModeV { vk::SamplerAddressMode::eRepeat };
    vk::Filter minFilter { vk::Filter::eLinear };
    vk::Filter magFilter { vk::Filter::eLinear };
    bool useMaxAnisotropy { true };
    bool anisotropyEnable { true };
    vk::BorderColor borderColor { vk::BorderColor::eIntOpaqueBlack };
    bool unnormalizedCoordinates { false };
    bool compareEnable { false };
    vk::CompareOp compareOp { vk::CompareOp::eAlways };
    vk::SamplerMipmapMode mipmapMode { vk::SamplerMipmapMode::eLinear };
    float mipLodBias { 0.0f };
    float minLod { 0.0f };
    float maxLod { 1.0f };
};

class SamplerResourceManager final : public ResourceManager<Sampler>
{
public:
    SamplerResourceManager(const std::shared_ptr<VulkanContext> context);
    ~SamplerResourceManager() final = default;

    NON_COPYABLE(SamplerResourceManager);
    NON_MOVABLE(SamplerResourceManager);

    ResourceHandle<Sampler> Create(const SamplerCreateInfo& creation);
    ResourceHandle<Sampler> Create(const Sampler& sampler) final { return ResourceManager<Sampler>::Create(sampler); }

    void Destroy(ResourceHandle<Sampler> handle) final;

private:
    std::shared_ptr<VulkanContext> _context;
};