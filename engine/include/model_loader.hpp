#pragma once

#include "mesh.hpp"
#include <string>

#include "lib/include_fastgltf.hpp"

class SingleTimeCommands;
class BatchBuffer;

class ModelLoader
{
public:
    ModelLoader(const VulkanBrain& brain);
    ~ModelLoader();

    NON_COPYABLE(ModelLoader);
    NON_MOVABLE(ModelLoader);

    ModelHandle Load(std::string_view path, BatchBuffer& batchBuffer);

    ResourceHandle<Mesh> LoadMesh(const StagingMesh::Primitive& stagingPrimitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer,
        ResourceHandle<Material> material);

    void ReadGeometrySize(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);

private:
    const VulkanBrain& _brain;
    fastgltf::Parser _parser;
    vk::UniqueSampler _sampler;
    ResourceHandle<Material> _defaultMaterial;

    StagingMesh::Primitive ProcessPrimitive(const fastgltf::Primitive& primitive, const fastgltf::Asset& gltf);
    ImageCreation ProcessImage(const fastgltf::Image& gltfImage, const fastgltf::Asset& gltf, std::vector<std::byte>& data, std::string_view name);
    MaterialCreation ProcessMaterial(const fastgltf::Material& gltfMaterial, const std::vector<ResourceHandle<Image>>& modelTextures, const fastgltf::Asset& gltf);

    vk::PrimitiveTopology MapGltfTopology(fastgltf::PrimitiveType gltfTopology);

    uint32_t MapTextureIndexToImageIndex(uint32_t textureIndex, const fastgltf::Asset& gltf);

    void CalculateTangents(StagingMesh::Primitive& stagingPrimitive);

    ModelHandle LoadModel(const fastgltf::Asset& gltf, BatchBuffer& batchBuffer, const std::string_view name);

    glm::vec4 CalculateTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
        glm::vec3 normal);

    ModelHandle LoadModel(const std::vector<Mesh>& meshes, const std::vector<ImageCreation>& textures,
        const std::vector<Material>& materials, BatchBuffer& batchBuffer, const fastgltf::Asset& gltf);

    void RecurseHierarchy(const fastgltf::Node& gltfNode, ModelHandle& hierarchy, const fastgltf::Asset& gltf,
        glm::mat4 matrix);

    Mesh::Primitive LoadPrimitive(const StagingMesh::Primitive& stagingPrimitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer,
        ResourceHandle<Material> material);
};
