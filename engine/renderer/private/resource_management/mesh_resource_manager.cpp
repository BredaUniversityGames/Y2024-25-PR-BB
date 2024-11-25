#include "resource_management/mesh_resource_manager.hpp"

#include "batch_buffer.hpp"
#include "gpu_resources.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"

template <>
std::weak_ptr<ResourceManager<GPUMesh>> ResourceHandle<GPUMesh>::manager = {};

ResourceHandle<GPUMesh> MeshResourceManager::Create(const CPUMesh& cpuMesh, std::vector<ResourceHandle<GPUMaterial>> materials)
{
    // todo: add fallback material.
    assert(materials.size() > 0);

    GPUMesh mesh;
    for (const auto& primitive : cpuMesh.primitives)
    {

        uint32_t correctedIndex = primitive.materialIndex;

        // Invalid index.
        if (primitive.materialIndex > materials.size())
        {
            correctedIndex = 0;
        }

        const ResourceHandle<GPUMaterial> material = materials.at(correctedIndex);
        mesh.primitives.emplace_back(CreatePrimitive(primitive, material));
    }

    return ResourceManager::Create(std::move(mesh));
}

ResourceHandle<GPUMesh> MeshResourceManager::Create(const CPUMesh::Primitive& data, ResourceHandle<GPUMaterial> material)
{
    GPUMesh mesh;
    mesh.primitives.emplace_back(CreatePrimitive(data, material));
    return ResourceManager::Create(std::move(mesh));
}

GPUMesh::Primitive MeshResourceManager::CreatePrimitive(const CPUMesh::Primitive& cpuPrimitive, ResourceHandle<GPUMaterial> material)
{
    GPUMesh::Primitive primitive;
    primitive.material = material;
    primitive.count = cpuPrimitive.indices.size();

    SingleTimeCommands commands { _vkContext };

    primitive.vertexOffset = _batchBuffer->AppendVertices(cpuPrimitive.vertices, commands);
    primitive.indexOffset = _batchBuffer->AppendIndices(cpuPrimitive.indices, commands);
    primitive.boundingRadius = glm::max(
        glm::distance(glm::vec3(0), cpuPrimitive.boundingBox.min),
        glm::distance(glm::vec3(0), cpuPrimitive.boundingBox.max));

    return primitive;
}
