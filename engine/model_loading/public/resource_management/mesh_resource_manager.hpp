#pragma once

#include "batch_buffer.hpp"
#include "common.hpp"
#include "cpu_resources.hpp"
#include "resource_manager.hpp"
#include "single_time_commands.hpp"
#include "vertex.hpp"
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

    template <typename TVertex>
    ResourceHandle<GPUMesh> Create(SingleTimeCommands& uploadCommands, const CPUMesh<TVertex>& cpuMesh, const std::vector<ResourceHandle<GPUMaterial>>& materials, BatchBuffer& batchBuffer)
    {
        uint32_t correctedIndex = cpuMesh.materialIndex;

        // Invalid index.
        if (cpuMesh.materialIndex > materials.size())
        {
            correctedIndex = 0;
        }

        const ResourceHandle<GPUMaterial> material = materials.at(correctedIndex);
        GPUMesh gpuMesh {};

        gpuMesh = CreateMesh(uploadCommands, cpuMesh, material, batchBuffer);
        return ResourceManager::Create(std::move(gpuMesh));
    }

    template <typename TVertex>
    ResourceHandle<GPUMesh> Create(SingleTimeCommands& uploadCommands, const CPUMesh<TVertex>& data, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer)
    {
        GPUMesh mesh = CreateMesh(uploadCommands, data, material, batchBuffer);
        return ResourceManager::Create(std::move(mesh));
    }

private:
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<VulkanContext> _vkContext;

    template <typename TVertex>
    GPUMesh CreateMesh(SingleTimeCommands& uploadCommands, const CPUMesh<TVertex>& cpuMesh, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer)
    {
        GPUMesh gpuMesh;
        gpuMesh.material = material;
        gpuMesh.count = cpuMesh.indices.empty() ? cpuMesh.vertices.size() : cpuMesh.indices.size();

        gpuMesh.vertexOffset = batchBuffer.AppendVertices(cpuMesh.vertices, uploadCommands);
        if (!cpuMesh.indices.empty())
        {
            gpuMesh.indexOffset = batchBuffer.AppendIndices(cpuMesh.indices, uploadCommands);
        }
        gpuMesh.boundingRadius = cpuMesh.boundingRadius;
        gpuMesh.boundingBox = cpuMesh.boundingBox;

        return gpuMesh;
    }
};
