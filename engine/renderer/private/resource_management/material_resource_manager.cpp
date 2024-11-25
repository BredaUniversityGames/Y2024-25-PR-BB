#include "resource_management/material_resource_manager.hpp"

#include "resource_management/image_resource_manager.hpp"

template <>
std::weak_ptr<ResourceManager<GPUMaterial>> ResourceHandle<GPUMaterial>::manager = {};

MaterialResourceManager::MaterialResourceManager(const std::shared_ptr<ImageResourceManager>& imageResourceManager)
    : _imageResourceManager(imageResourceManager)
{
}

ResourceHandle<GPUMaterial> MaterialResourceManager::Create(const MaterialCreation& creation)
{
    return ResourceManager::Create(GPUMaterial { creation, _imageResourceManager });
}
