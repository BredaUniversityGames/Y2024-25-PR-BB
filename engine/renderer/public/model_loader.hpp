#pragma once

#include "mesh.hpp"
#include "model.hpp"

#include <lib/include_fastgltf.hpp>
#include <memory>
#include <vector>

class SingleTimeCommands;
class BatchBuffer;
class ECS;
class GraphicsContext;

class ModelLoader
{
public:
    ModelLoader(const std::shared_ptr<GraphicsContext>& context, std::shared_ptr<const ECS> ecs);
    ~ModelLoader();

    enum class LoadMode : uint8_t
    {
        eFlat,
        eHierarchical
    };

    NON_COPYABLE(ModelLoader);
    NON_MOVABLE(ModelLoader);

   
    ModelData ExtractModelFromFile(std::string_view path, LoadMode loadMode);

    ResourceHandle<Mesh> LoadMesh(const StagingMesh::Primitive& stagingPrimitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer,
        ResourceHandle<Material> material);

    void ReadGeometrySize(std::string_view path, uint32_t& vertexBufferSize, uint32_t& indexBufferSize);

private:
    std::shared_ptr<GraphicsContext> _context;
    std::shared_ptr<const ECS> _ecs;
    fastgltf::Parser _parser;
    ResourceHandle<Material> _defaultMaterial;

    StagingMesh::Primitive ProcessPrimitive(const fastgltf::Primitive& primitive, const fastgltf::Asset& gltf);
    ImageCreation ProcessImage(const fastgltf::Image& gltfImage, const fastgltf::Asset& gltf, std::vector<std::byte>& data, std::string_view name);
    MaterialCreation ProcessMaterial(const fastgltf::Material& gltfMaterial, const std::vector<ResourceHandle<Image>>& modelTextures, const fastgltf::Asset& gltf);

    vk::PrimitiveTopology MapGltfTopology(fastgltf::PrimitiveType gltfTopology);

    uint32_t MapTextureIndexToImageIndex(uint32_t textureIndex, const fastgltf::Asset& gltf);

    void CalculateTangents(StagingMesh::Primitive& stagingPrimitive);

    Model LoadModel(const fastgltf::Asset& gltf, BatchBuffer& batchBuffer, const std::string_view name, LoadMode loadMode);

    glm::vec4 CalculateTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2,
        glm::vec3 normal);

    Model LoadModel(const std::vector<Mesh>& meshes, const std::vector<ImageCreation>& textures,
        const std::vector<Material>& materials, BatchBuffer& batchBuffer, const fastgltf::Asset& gltf);

    void RecurseHierarchy(const fastgltf::Node& gltfNode, Model& model, const fastgltf::Asset& gltf, Hierarchy::Node& parent);

    Mesh::Primitive LoadPrimitive(const StagingMesh::Primitive& stagingPrimitive, SingleTimeCommands& commandBuffer, BatchBuffer& batchBuffer,
        ResourceHandle<Material> material);
};
