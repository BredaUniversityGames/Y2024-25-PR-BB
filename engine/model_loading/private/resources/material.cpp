#include "resources/material.hpp"

GPUMaterial::GPUMaterial(const MaterialCreation& creation, const std::shared_ptr<ResourceManager<GPUImage>>& imageResourceManager)
    : _imageResourceManager(imageResourceManager)
{
    albedoMap = creation.albedoMap;
    mrMap = creation.metallicRoughnessMap;
    normalMap = creation.normalMap;
    occlusionMap = creation.occlusionMap;
    emissiveMap = creation.emissiveMap;

    gpuInfo.useAlbedoMap = _imageResourceManager->IsValid(albedoMap);
    gpuInfo.useMRMap = _imageResourceManager->IsValid(mrMap);
    gpuInfo.useNormalMap = _imageResourceManager->IsValid(normalMap);
    gpuInfo.useOcclusionMap = _imageResourceManager->IsValid(occlusionMap);
    gpuInfo.useEmissiveMap = _imageResourceManager->IsValid(emissiveMap);

    gpuInfo.albedoMapIndex = albedoMap.Index();
    gpuInfo.mrMapIndex = mrMap.Index();
    gpuInfo.normalMapIndex = normalMap.Index();
    gpuInfo.occlusionMapIndex = occlusionMap.Index();
    gpuInfo.emissiveMapIndex = emissiveMap.Index();

    gpuInfo.albedoFactor = creation.albedoFactor;
    gpuInfo.metallicFactor = creation.metallicFactor;
    gpuInfo.roughnessFactor = creation.roughnessFactor;
    gpuInfo.normalScale = creation.normalScale;
    gpuInfo.occlusionStrength = creation.occlusionStrength;
    gpuInfo.emissiveFactor = creation.emissiveFactor;
}

GPUMaterial::~GPUMaterial()
{
}