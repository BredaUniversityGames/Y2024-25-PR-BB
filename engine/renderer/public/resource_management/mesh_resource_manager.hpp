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
struct Mesh;

class MeshResourceManager final : public ResourceManager<Mesh>
{
public:
    MeshResourceManager() = default;
    ~MeshResourceManager() = default;

    // deferred init
    void SetBatchBuffer(std::shared_ptr<BatchBuffer> batchBuffer) { _batchBuffer = batchBuffer; };

    ResourceHandle<Mesh> Create(SingleTimeCommands& commandBuffer, const CPUResources::Mesh& data, std::vector<ResourceHandle<Material>> materials = {});
    ResourceHandle<Mesh> Create(SingleTimeCommands& commandBuffer, const CPUResources::Mesh::Primitive& data, ResourceHandle<Material> material);

private:
    std::shared_ptr<BatchBuffer> _batchBuffer;
    Mesh::Primitive CreatePrimitive(SingleTimeCommands& commandBuffer, const CPUResources::Mesh::Primitive& data, ResourceHandle<Material> material);
};
