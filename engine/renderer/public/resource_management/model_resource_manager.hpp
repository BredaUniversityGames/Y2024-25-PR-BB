#pragma once

#include <memory>
#include <vulkan/vulkan.hpp>

#include "common.hpp"
#include "model.hpp"
#include "resource_manager.hpp"

class SingleTimeCommands;
class BatchBuffer;
class VulkanContext;
struct GPUMesh;

class ModelResourceManager final : public ResourceManager<GPUModel>
{
public:
    ModelResourceManager(std::shared_ptr<ImageResourceManager> imageResourceManager,
        std::shared_ptr<MaterialResourceManager> materialResourceManager,
        std::shared_ptr<MeshResourceManager> meshResourceManager);

    ResourceHandle<GPUModel> Create(const CPUModel& data);

    ModelResourceManager() = default;

private:
    std::shared_ptr<ImageResourceManager> _imageResourceManager;
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<MeshResourceManager> _meshResourceManager;
};
