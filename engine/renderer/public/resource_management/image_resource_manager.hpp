#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"
#include <memory>

class VulkanContext;

class ImageResourceManager final : public ResourceManager<GPUImage>
{
public:
    explicit ImageResourceManager(const std::shared_ptr<VulkanContext>& context, ResourceHandle<Sampler> defaultSampler);

    ResourceHandle<GPUImage> Create(const CPUImage& cpuImage, ResourceHandle<Sampler> sampler);

    // Create an image with the default sampler
    ResourceHandle<GPUImage> Create(const CPUImage& cpuImage)
    {
        return Create(cpuImage, _defaultSampler);
    }

private:
    std::shared_ptr<VulkanContext> _context;

    ResourceHandle<Sampler> _defaultSampler;

    vk::ImageType ImageTypeConversion(ImageType type);
    vk::ImageViewType ImageViewTypeConversion(ImageType type);
};
