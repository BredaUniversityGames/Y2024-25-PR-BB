#include "resource_management/material_resource_manager.hpp"

#include "resource_management/image_resource_manager.hpp"

MaterialResourceManager::MaterialResourceManager(const std::shared_ptr<ImageResourceManager>& imageResourceManager)
    : _imageResourceManager(imageResourceManager)
{
}

ResourceHandle<Material> MaterialResourceManager::Create(const MaterialCreation& creation)
{
    Material materialResource {};

    materialResource.albedoMap = creation.albedoMap;
    materialResource.mrMap = creation.metallicRoughnessMap;
    materialResource.normalMap = creation.normalMap;
    materialResource.occlusionMap = creation.occlusionMap;
    materialResource.emissiveMap = creation.emissiveMap;

    Material::GPUInfo& gpuInfo = materialResource.gpuInfo;
    gpuInfo.useAlbedoMap = _imageResourceManager->IsValid(materialResource.albedoMap);
    gpuInfo.useMRMap = _imageResourceManager->IsValid(materialResource.mrMap);
    gpuInfo.useNormalMap = _imageResourceManager->IsValid(materialResource.normalMap);
    gpuInfo.useOcclusionMap = _imageResourceManager->IsValid(materialResource.occlusionMap);
    gpuInfo.useEmissiveMap = _imageResourceManager->IsValid(materialResource.emissiveMap);

    gpuInfo.albedoMapIndex = materialResource.albedoMap.index;
    gpuInfo.mrMapIndex = materialResource.mrMap.index;
    gpuInfo.normalMapIndex = materialResource.normalMap.index;
    gpuInfo.occlusionMapIndex = materialResource.occlusionMap.index;
    gpuInfo.emissiveMapIndex = materialResource.emissiveMap.index;

    gpuInfo.albedoFactor = creation.albedoFactor;
    gpuInfo.metallicFactor = creation.metallicFactor;
    gpuInfo.roughnessFactor = creation.roughnessFactor;
    gpuInfo.normalScale = creation.normalScale;
    gpuInfo.occlusionStrength = creation.occlusionStrength;
    gpuInfo.emissiveFactor = creation.emissiveFactor;

    return ResourceManager::Create(materialResource);
}
