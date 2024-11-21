#include "resource_management/mesh_resource_manager.hpp"

#include "batch_buffer.hpp"
#include "gpu_resources.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"

template <>
std::weak_ptr<ResourceManager<Mesh>> ResourceHandle<Mesh>::manager = {};

ResourceHandle<Mesh> MeshResourceManager::Create(SingleTimeCommands& commandBuffer, const CPUResources::Mesh& cpuMesh, std::vector<ResourceHandle<Material>> materials)
{

    Mesh mesh;
    for (const auto& primitive : cpuMesh.primitives)
    {
        ResourceHandle<Material> material = primitive.materialIndex.has_value() ? materials.at(primitive.materialIndex.value()) : ResourceHandle<Material>::Null();
        mesh.primitives.emplace_back(CreatePrimitive(commandBuffer, primitive, material));
    }

    return ResourceManager::Create(std::move(mesh));
}

ResourceHandle<Mesh> MeshResourceManager::Create(SingleTimeCommands& commandBuffer, const CPUResources::Mesh::Primitive& data, ResourceHandle<Material> material)
{
    Mesh mesh;
    mesh.primitives.emplace_back(CreatePrimitive(commandBuffer, data, material));
    return ResourceManager::Create(std::move(mesh));
}

Mesh::Primitive MeshResourceManager::CreatePrimitive(SingleTimeCommands& commandBuffer, const CPUResources::Mesh::Primitive& cpuPrimitive, ResourceHandle<Material> material)
{
    Mesh::Primitive primitive;
    primitive.material = material;
    primitive.count = cpuPrimitive.indices.size();
    primitive.vertexOffset = _batchBuffer->AppendVertices(cpuPrimitive.vertices, commandBuffer);
    primitive.indexOffset = _batchBuffer->AppendIndices(cpuPrimitive.indices, commandBuffer);
    primitive.boundingRadius = glm::max(
        glm::distance(glm::vec3(0), cpuPrimitive.boundingBox.min),
        glm::distance(glm::vec3(0), cpuPrimitive.boundingBox.max));

    return primitive;
}
