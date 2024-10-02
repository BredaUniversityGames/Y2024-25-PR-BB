#pragma once

#include "mesh.hpp"
#include <string>

#undef None
#include <fastgltf/core.hpp>

class SingleTimeCommands;

class ModelLoader
{
public:
    ModelLoader(const VulkanBrain& brain);
    ~ModelLoader();

    NON_COPYABLE(ModelLoader);
    NON_MOVABLE(ModelLoader);

    ModelHandle Load(std::string_view path);
    MeshPrimitiveHandle LoadPrimitive(const MeshPrimitive& primitive, SingleTimeCommands& commandBuffer, ResourceHandle<Material> material);

private:
    const VulkanBrain& _brain;
    fastgltf::Parser _parser;
    vk::UniqueSampler _sampler;
    ResourceHandle<Material> _defaultMaterial;

    MeshPrimitive ProcessPrimitive(const fastgltf::Primitive& primitive, const fastgltf::Asset& gltf);
    ImageCreation ProcessImage(const fastgltf::Image& gltfImage, const fastgltf::Asset& gltf, std::vector<std::byte>& data, std::string_view name);
    MaterialCreation ProcessMaterial(const fastgltf::Material& gltfMaterial, const std::vector<ResourceHandle<Image>>& modelTextures, const fastgltf::Asset& gltf);

    vk::PrimitiveTopology MapGltfTopology(fastgltf::PrimitiveType gltfTopology);
    vk::IndexType MapIndexType(fastgltf::ComponentType componentType);
    uint32_t MapTextureIndexToImageIndex(uint32_t textureIndex, const fastgltf::Asset& gltf);

    void CalculateTangents(MeshPrimitive& primitive);
    glm::vec4 CalculateTangent(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec3 normal);
    ModelHandle LoadModel(const fastgltf::Asset& gltf, const std::string_view name);

    void RecurseHierarchy(const fastgltf::Node& gltfNode, ModelHandle& hierarchy, const fastgltf::Asset& gltf, glm::mat4 matrix);
};
