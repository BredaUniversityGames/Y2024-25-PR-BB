#pragma once

#include "cpu_resources.hpp"
#include "resource_manager.hpp"
class ImageResourceManager;
class MaterialResourceManager;
class MeshResourceManager;
class BatchBuffer;

#include <memory>

class ModelResourceManager final : public ResourceManager<GPUModel>
{
public:
    ModelResourceManager(std::shared_ptr<VulkanContext> vkContext,
        std::shared_ptr<ImageResourceManager> imageResourceManager,
        std::shared_ptr<MaterialResourceManager> materialResourceManager,
        std::shared_ptr<MeshResourceManager> meshResourceManager);

    ResourceHandle<GPUModel> Create(const CPUModel& data, BatchBuffer& staticBatchBuffer, BatchBuffer& skinnedBatchBuffer);

    ModelResourceManager() = default;

private:
    std::shared_ptr<ImageResourceManager> _imageResourceManager;
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<MeshResourceManager> _meshResourceManager;
    std::shared_ptr<VulkanContext> _vkContext;
};
