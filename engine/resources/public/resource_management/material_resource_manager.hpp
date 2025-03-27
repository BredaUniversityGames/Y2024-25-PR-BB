#pragma once

#include "resource_manager.hpp"
#include "resources/material.hpp"

#include <memory>

class ImageResourceManager;

class MaterialResourceManager final : public ResourceManager<GPUMaterial>
{
public:
    explicit MaterialResourceManager(const std::shared_ptr<ImageResourceManager>& ImageResourceManager);
    ResourceHandle<GPUMaterial> Create(const MaterialCreation& creation);

private:
    std::shared_ptr<ImageResourceManager> _imageResourceManager;
};