#pragma once

#include "batch_buffer.hpp"
#include "common.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "resource_manager.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

class MaterialResourceManager;

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

    template <typename TVertex>
    GPUMesh::Primitive CreatePrimitive(const CPUMesh::Primitive<TVertex>& cpuPrimitive, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer)
    {
        GPUMesh::Primitive primitive;
        primitive.material = material;
        primitive.count = cpuPrimitive.indices.size();

        SingleTimeCommands commands { _vkContext };

        primitive.vertexOffset = batchBuffer.AppendVertices(cpuPrimitive.vertices, commands);
        primitive.indexOffset = batchBuffer.AppendIndices(cpuPrimitive.indices, commands);
        primitive.boundingRadius = glm::max(
            glm::distance(glm::vec3 { 0 }, cpuPrimitive.boundingBox.min),
            glm::distance(glm::vec3 { 0 }, cpuPrimitive.boundingBox.max));

        return primitive;
    }
};
