#include "resource_management/model_resource_manager.hpp"

#include "batch_buffer.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "single_time_commands.hpp"

template <>
std::weak_ptr<ResourceManager<GPUResources::Model>> ResourceHandle<GPUResources::Model>::manager = {};

ModelResourceManager::ModelResourceManager(std::shared_ptr<ImageResourceManager> imageResourceManager, std::shared_ptr<MaterialResourceManager> materialResourceManager, std::shared_ptr<MeshResourceManager> meshResourceManager)
    : _imageResourceManager(imageResourceManager)
    , _materialResourceManager(materialResourceManager)
    , _meshResourceManager(meshResourceManager)
{
}

ResourceHandle<GPUResources::Model>
ModelResourceManager::Create(const CPUResources::ModelData& data, SingleTimeCommands& commandBuffer)
{
    GPUResources::Model model;

    for (const auto& i : data.textures)
    {
        model.textures.emplace_back(_imageResourceManager->Create(i));
    }

    for (const auto& i : data.materials)
    {
        MaterialCreation material {};

        material.albedoFactor = i.albedoFactor;
        if (i.albedoMap.has_value())
            material.albedoMap = model.textures[i.albedoMap.value()];

        material.metallicFactor = i.metallicFactor;
        material.roughnessFactor = i.roughnessFactor;
        material.metallicRoughnessUVChannel = i.metallicRoughnessUVChannel;
        if (i.metallicRoughnessMap.has_value())
            material.metallicRoughnessMap = model.textures[i.metallicRoughnessMap.value()];

        material.emissiveFactor = i.emissiveFactor;
        if (i.emissiveMap.has_value())
            material.emissiveMap = model.textures[i.emissiveMap.value()];

        material.occlusionStrength = i.occlusionStrength;
        material.occlusionUVChannel = i.occlusionUVChannel;
        if (i.occlusionMap.has_value())
            material.occlusionMap = model.textures[i.occlusionMap.value()];

        model.materials.emplace_back(_materialResourceManager->Create(material));
    }

    for (const auto& i : data.meshes)
    {
        _meshResourceManager->Create(commandBuffer, i, model.materials);
    }

    return ResourceManager::Create(std::move(model));
}