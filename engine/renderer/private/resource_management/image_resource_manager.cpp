#include "resource_management/image_resource_manager.hpp"

#include "../vulkan_helper.hpp"
#include "log.hpp"
#include "vulkan_context.hpp"

template <>
std::weak_ptr<ResourceManager<Image>> ResourceHandle<Image>::manager = {};

ImageResourceManager::ImageResourceManager(const std::shared_ptr<VulkanContext>& context)
    : _context(context)
{
}

ResourceHandle<Image> ImageResourceManager::Create(const ImageCreation& creation)
{
    return ResourceManager::Create(Image { creation, _context });
}

vk::ImageType ImageResourceManager::ImageTypeConversion(ImageType type)
{
    switch (type)
    {
    case ImageType::e2D:
    case ImageType::e2DArray:
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
    case ImageType::e2D:
        return vk::ImageViewType::e2D;
    case ImageType::e2DArray:
        return vk::ImageViewType::e2DArray;
    case ImageType::eCubeMap:
        return vk::ImageViewType::eCube;
    default:
        throw std::runtime_error("Unsupported ImageType!");
    }
}
