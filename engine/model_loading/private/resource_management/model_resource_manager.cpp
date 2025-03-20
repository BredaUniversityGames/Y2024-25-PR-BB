#include "resource_management/model_resource_manager.hpp"

#include "batch_buffer.hpp"
#include "cpu_resources.hpp"
#include "profile_macros.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"

ModelResourceManager::ModelResourceManager(std::shared_ptr<VulkanContext> vkContext, std::shared_ptr<ImageResourceManager> imageResourceManager, std::shared_ptr<MaterialResourceManager> materialResourceManager, std::shared_ptr<MeshResourceManager> meshResourceManager)
    : _imageResourceManager(imageResourceManager)
    , _materialResourceManager(materialResourceManager)
    , _meshResourceManager(meshResourceManager)
    , _vkContext(vkContext)
{
}

ResourceHandle<GPUModel> ModelResourceManager::Create(const CPUModel& data, BatchBuffer& staticBatchBuffer, BatchBuffer& skinnedBatchBuffer)
{
    GPUModel model {};
    SingleTimeCommands commands { _vkContext };

    {
        ZoneScopedN("Texture upload dispatch");
        for (const auto& image : data.textures)
        {
            model.textures.emplace_back(_imageResourceManager->Create(image, &commands));
        }
    }

    {
        ZoneScopedN("Material upload dispatch");
        for (const auto& cpuMaterial : data.materials)
        {
            MaterialCreation materialCreation {};

            materialCreation.normalScale = cpuMaterial.normalScale;
            if (cpuMaterial.normalMap.has_value())
                materialCreation.normalMap = model.textures[cpuMaterial.normalMap.value()];

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
    }

    {
        ZoneScopedN("Mesh upload dispatch");
        for (const auto& cpuMesh : data.meshes)
        {
            model.staticMeshes.emplace_back(_meshResourceManager->Create(commands, cpuMesh, model.materials, staticBatchBuffer));
        }

        for (const auto& cpuMesh : data.skinnedMeshes)
        {
            model.skinnedMeshes.emplace_back(_meshResourceManager->Create(commands, cpuMesh, model.materials, skinnedBatchBuffer));
        }
    }

    {
        ZoneScopedN("Wait until uploaded");
        commands.Submit();
    }

    return ResourceManager::Create(std::move(model));
}
