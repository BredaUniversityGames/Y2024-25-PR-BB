#include "resource_management/model_resource_manager.hpp"

#include "batch_buffer.hpp"
#include "model.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"

template <>
std::weak_ptr<ResourceManager<GPUModel>> ResourceHandle<GPUModel>::manager = {};

ModelResourceManager::ModelResourceManager(std::shared_ptr<ImageResourceManager> imageResourceManager, std::shared_ptr<MaterialResourceManager> materialResourceManager, std::shared_ptr<MeshResourceManager> meshResourceManager)
    : _imageResourceManager(imageResourceManager)
    , _materialResourceManager(materialResourceManager)
    , _meshResourceManager(meshResourceManager)
{
}

ResourceHandle<GPUModel>
ModelResourceManager::Create(const CPUModel& data, BatchBuffer& batchBuffer)
{
    GPUModel model;

    for (const auto& image : data.textures)
    {

        model.textures.emplace_back(_imageResourceManager->Create(image));
    }

    for (const auto& cpuMaterial : data.materials)
    {
        MaterialCreation materialCreation {};

        materialCreation.albedoFactor = cpuMaterial.albedoFactor;
        if (cpuMaterial.albedoMap.has_value())
            materialCreation.albedoMap = model.textures[cpuMaterial.albedoMap.value()];

        materialCreation.metallicFactor = cpuMaterial.metallicFactor;
        materialCreation.roughnessFactor = cpuMaterial.roughnessFactor;
        materialCreation.metallicRoughnessUVChannel = cpuMaterial.metallicRoughnessUVChannel;
        if (cpuMaterial.metallicRoughnessMap.has_value())
            materialCreation.metallicRoughnessMap = model.textures[cpuMaterial.metallicRoughnessMap.value()];

        materialCreation.emissiveFactor = cpuMaterial.emissiveFactor;
        if (cpuMaterial.emissiveMap.has_value())
            materialCreation.emissiveMap = model.textures[cpuMaterial.emissiveMap.value()];

        materialCreation.occlusionStrength = cpuMaterial.occlusionStrength;
        materialCreation.occlusionUVChannel = cpuMaterial.occlusionUVChannel;
        if (cpuMaterial.occlusionMap.has_value())
            materialCreation.occlusionMap = model.textures[cpuMaterial.occlusionMap.value()];

        model.materials.emplace_back(_materialResourceManager->Create(materialCreation));
    }

    for (const auto& cpuMesh : data.meshes)
    {
        model.meshes.emplace_back(_meshResourceManager->Create(cpuMesh, model.materials, batchBuffer));
    }

    return ResourceManager::Create(std::move(model));
}