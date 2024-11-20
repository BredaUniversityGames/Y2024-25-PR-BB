#include "resource_management/material_resource_manager.hpp"

#include "resource_management/image_resource_manager.hpp"

template <>
std::weak_ptr<ResourceManager<Material>> ResourceHandle<Material>::manager = {};

MaterialResourceManager::MaterialResourceManager(const std::shared_ptr<ImageResourceManager>& imageResourceManager)
    : _imageResourceManager(imageResourceManager)
{
}

ResourceHandle<Material> MaterialResourceManager::Create(const MaterialCreation& creation)
{
    return ResourceManager::Create(Material { creation, _imageResourceManager });
}
