#pragma once

#include "model.hpp"
#include "resource_manager.hpp"
#include <memory>

class ModelResourceManager final : public ResourceManager<GPUModel>
{
public:
    ModelResourceManager(std::shared_ptr<ImageResourceManager> imageResourceManager,
        std::shared_ptr<MaterialResourceManager> materialResourceManager,
        std::shared_ptr<MeshResourceManager> meshResourceManager);

    ResourceHandle<GPUModel> Create(const CPUModel& data, BatchBuffer& batchBuffer);

    ModelResourceManager() = default;

private:
    std::shared_ptr<ImageResourceManager> _imageResourceManager;
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<MeshResourceManager> _meshResourceManager;
};
