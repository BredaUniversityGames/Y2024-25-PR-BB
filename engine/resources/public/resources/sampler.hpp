#pragma once
#include "vulkan_context.hpp"

struct SamplerCreation
{
    SamplerCreation& SetGlobalAddressMode(vk::SamplerAddressMode addressMode);

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
    vk::SamplerReductionMode reductionMode { vk::SamplerReductionMode::eWeightedAverage };
};

struct Sampler
{
    Sampler(const SamplerCreation& creation, const std::shared_ptr<VulkanContext>& context);
    ~Sampler();

    Sampler(Sampler&& other) noexcept;
    Sampler& operator=(Sampler&& other) noexcept;

    NON_COPYABLE(Sampler);

    vk::Sampler sampler;

private:
    std::shared_ptr<VulkanContext> _context;
};