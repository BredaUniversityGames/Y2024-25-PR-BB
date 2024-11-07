#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

#include <memory>

class VulkanContext;

class ImageResourceManager : public ResourceManager<Image>
{
public:
    explicit ImageResourceManager(const std::shared_ptr<VulkanContext>& context);
    ResourceHandle<Image> Create(const ImageCreation& creation);
    ResourceHandle<Image> Create(const Image& image) override { return ResourceManager<Image>::Create(image); }
    void Destroy(ResourceHandle<Image> handle) override;

private:
    std::shared_ptr<VulkanContext> _context;

    vk::ImageType ImageTypeConversion(ImageType type);
    vk::ImageViewType ImageViewTypeConversion(ImageType type);
};
