#pragma once

#include "batch_buffer.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

#include "common.hpp"
#include "model.hpp"

class BatchBuffer;
class SingleTimeCommands;

#include "resource_manager.hpp"
class VulkanContext;
struct GPUMesh;

class MeshResourceManager final : public ResourceManager<GPUMesh>
{
public:
    MeshResourceManager(std::shared_ptr<VulkanContext> context)
        : _vkContext(context)
    {
    }
    ~MeshResourceManager() = default;

    // deferred init
    void SetBatchBuffer(std::shared_ptr<BatchBuffer> batchBuffer) { _batchBuffer = batchBuffer; };

    ResourceHandle<GPUMesh> Create(const CPUMesh& data, std::vector<ResourceHandle<GPUMaterial>> materials = {});
    ResourceHandle<GPUMesh> Create(const CPUMesh::Primitive& data, ResourceHandle<GPUMaterial> material);

private:
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<BatchBuffer> _batchBuffer;
    std::shared_ptr<VulkanContext> _vkContext;
    GPUMesh::Primitive CreatePrimitive(const CPUMesh::Primitive& data, ResourceHandle<GPUMaterial> material);
};
