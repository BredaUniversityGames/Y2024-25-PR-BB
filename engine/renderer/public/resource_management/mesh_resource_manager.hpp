#pragma once

#include "common.hpp"
#include "model.hpp"
#include "resource_manager.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>

class BatchBuffer;
class SingleTimeCommands;
class VulkanContext;
class MaterialResourceManager;
struct GPUMesh;

class MeshResourceManager final : public ResourceManager<GPUMesh>
{
public:
    MeshResourceManager(const std::shared_ptr<VulkanContext>& context)
        : _vkContext(context)
    {
    }
    ~MeshResourceManager() = default;

    ResourceHandle<GPUMesh> Create(const CPUMesh& data, const std::vector<ResourceHandle<GPUMaterial>>& materials, BatchBuffer& batchBuffer);
    ResourceHandle<GPUMesh> Create(const CPUMesh::Primitive<Vertex>& data, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer);

private:
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<VulkanContext> _vkContext;
    GPUMesh::Primitive CreatePrimitive(const CPUMesh::Primitive<Vertex>& data, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer);
};
