#include "resource_management/image_resource_manager.hpp"

#include "../vulkan_helper.hpp"
#include "log.hpp"
#include "vulkan_context.hpp"

ImageResourceManager::ImageResourceManager(const std::shared_ptr<VulkanContext>& context, ResourceHandle<Sampler> defaultSampler)
    : _context(context)
    , _defaultSampler(defaultSampler)
{
}

ResourceHandle<GPUImage> ImageResourceManager::Create(const CPUImage& cpuImage, ResourceHandle<Sampler> sampler, SingleTimeCommands* const commands)
{
    return ResourceManager::Create(GPUImage { cpuImage, sampler, _context, commands });
}

vk::ImageType ImageResourceManager::ImageTypeConversion(ImageType type)
{
    switch (type)
    {
    case ImageType::e2D:
    case ImageType::eDepth:
    case ImageType::eShadowMap:
    case ImageType::eCubeMap:
        return vk::ImageType::e2D;
    default:
        throw std::runtime_error("Unsupported ImageType!");
    }
}

vk::ImageViewType ImageResourceManager::ImageViewTypeConversion(ImageType type)
{
    switch (type)
    {
    case ImageType::eShadowMap:
    case ImageType::eDepth:
    case ImageType::e2D:
        return vk::ImageViewType::e2D;
    case ImageType::eCubeMap:
        return vk::ImageViewType::eCube;
    default:
        throw std::runtime_error("Unsupported ImageType!");
    }
}
