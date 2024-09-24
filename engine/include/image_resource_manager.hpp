#pragma once

#include "resource_manager.hpp"
#include "gpu_resources.hpp"

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
