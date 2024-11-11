#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

#include <memory>

class ImageResourceManager;

class MaterialResourceManager final : public ResourceManager<Material>
{
public:
    explicit MaterialResourceManager(const std::shared_ptr<ImageResourceManager>& ImageResourceManager);
    ~MaterialResourceManager() final = default;

    ResourceHandle<Material> Create(const MaterialCreation& creation);
    ResourceHandle<Material> Create(const Material& material) final { return ResourceManager<Material>::Create(material); }

private:
    std::shared_ptr<ImageResourceManager> _imageResourceManager;
};