#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "common.hpp"
#include "model.hpp"
#include "resource_manager.hpp"

class SingleTimeCommands;
class BatchBuffer;
class VulkanContext;
struct Mesh;

class ModelResourceManager final : public ResourceManager<GPUResources::Model>
{
public:
    ModelResourceManager(std::shared_ptr<ImageResourceManager> imageResourceManager,
        std::shared_ptr<MaterialResourceManager> materialResourceManager,
        std::shared_ptr<MeshResourceManager> meshResourceManager);

    ResourceHandle<GPUResources::Model> Create(const CPUResources::ModelData& data, SingleTimeCommands& commandBuffer);

    ModelResourceManager() = default;

private:
    std::shared_ptr<ImageResourceManager> _imageResourceManager;
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<MeshResourceManager> _meshResourceManager;
};
