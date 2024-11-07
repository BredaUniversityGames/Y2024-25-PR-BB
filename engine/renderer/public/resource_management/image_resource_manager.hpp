#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanContext;

class ImageResourceManager : public ResourceManager<Image>
{
public:
    explicit ImageResourceManager(const VulkanContext& brain);
    ResourceHandle<Image> Create(const ImageCreation& creation);
    ResourceHandle<Image> Create(const Image& image) override { return ResourceManager<Image>::Create(image); }
    void Destroy(ResourceHandle<Image> handle) override;

private:
    const VulkanContext& _brain;

    vk::ImageType ImageTypeConversion(ImageType type);
    vk::ImageViewType ImageViewTypeConversion(ImageType type);
};
