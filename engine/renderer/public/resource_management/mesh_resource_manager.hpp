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

#include <iostream>

class MaterialResourceManager;

class MeshResourceManager final : public ResourceManager<GPUMesh>
{
public:
    MeshResourceManager(const std::shared_ptr<VulkanContext>& context)
        : _vkContext(context)
    {
    }
    ~MeshResourceManager() = default;

    const std::unordered_map<std::string, ResourceHandle<GPUMesh>>& ViewLoadedMeshes()
    {
        return _loadedMeshes;
    }

    ResourceHandle<GPUMesh> TryGetMesh(const std::string_view mesh)
    {
        const auto resource = _loadedMeshes.find(mesh.data());

        return resource != _loadedMeshes.end() ? resource->second : _loadedMeshes.begin()->second;
    }

    template <typename TVertex>
    ResourceHandle<GPUMesh> Create(const CPUMesh<TVertex>& cpuMesh, const std::vector<ResourceHandle<GPUMaterial>>& materials, BatchBuffer& batchBuffer)
    {
        uint32_t correctedIndex = cpuMesh.materialIndex;

        // Invalid index.
        if (cpuMesh.materialIndex > materials.size())
        {
            correctedIndex = 0;
        }

        const ResourceHandle<GPUMaterial> material = materials.at(correctedIndex);

        GPUMesh gpuMesh = CreateMesh(cpuMesh, material, batchBuffer);

        ResourceHandle<GPUMesh> resource = ResourceManager::Create(std::move(gpuMesh));

        _loadedMeshes[cpuMesh.name] = resource;

        return resource;
    }

    template <typename TVertex>
    ResourceHandle<GPUMesh> Create(const CPUMesh<TVertex>& data, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer)
    {
        GPUMesh mesh = CreateMesh(data, material, batchBuffer);

        ResourceHandle<GPUMesh> resource = ResourceManager::Create(std::move(mesh));

        _loadedMeshes[data.name] = resource;

        return resource;
    }

private:
    std::shared_ptr<MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<VulkanContext> _vkContext;

    std::unordered_map<std::string, ResourceHandle<GPUMesh>> _loadedMeshes;

    template <typename TVertex>
    GPUMesh CreateMesh(const CPUMesh<TVertex>& cpuMesh, ResourceHandle<GPUMaterial> material, BatchBuffer& batchBuffer)
    {
        GPUMesh gpuMesh;
        gpuMesh.material = material;
        gpuMesh.count = cpuMesh.indices.empty() ? cpuMesh.vertices.size() : cpuMesh.indices.size();

        SingleTimeCommands commands { _vkContext };

        gpuMesh.vertexOffset = batchBuffer.AppendVertices(cpuMesh.vertices, commands);
        if (!cpuMesh.indices.empty())
        {
            gpuMesh.indexOffset = batchBuffer.AppendIndices(cpuMesh.indices, commands);
        }
        gpuMesh.boundingRadius = cpuMesh.boundingRadius;
        gpuMesh.boundingBox = cpuMesh.boundingBox;

        return gpuMesh;
    }
};
