#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanBrain;

class ImageResourceManager : public ResourceManager<Image>
{
public:
    explicit ImageResourceManager(const VulkanBrain& brain);
    ResourceHandle<Image> Create(const ImageCreation& creation);
    ResourceHandle<Image> Create(const Image& image) override { return ResourceManager<Image>::Create(image); }
    void Destroy(ResourceHandle<Image> handle) override;

private:
    const VulkanBrain& _brain;

    vk::ImageType ImageTypeConversion(ImageType type);
    vk::ImageViewType ImageViewTypeConversion(ImageType type);
};
