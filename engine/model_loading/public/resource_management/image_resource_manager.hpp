#pragma once

#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "single_time_commands.hpp"

#include <memory>

class VulkanContext;

class ImageResourceManager final : public ResourceManager<GPUImage>
{
public:
    explicit ImageResourceManager(const std::shared_ptr<VulkanContext>& context, ResourceHandle<Sampler> defaultSampler);

    ResourceHandle<GPUImage> Create(const CPUImage& cpuImage, ResourceHandle<Sampler> sampler, SingleTimeCommands* const commands = nullptr);

    // Create an image with the default sampler
    ResourceHandle<GPUImage> Create(const CPUImage& cpuImage, SingleTimeCommands* commands = nullptr)
    {
        return Create(cpuImage, _defaultSampler, commands);
    }

private:
    std::shared_ptr<VulkanContext> _context;

    ResourceHandle<Sampler> _defaultSampler;

    vk::ImageType ImageTypeConversion(ImageType type);
    vk::ImageViewType ImageViewTypeConversion(ImageType type);
};
