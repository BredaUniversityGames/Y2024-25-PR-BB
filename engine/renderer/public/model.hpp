#pragma once

#include "mesh.hpp"
#include <glm/mat4x4.hpp>
#include <vector>

struct ModelResources
{
    std::vector<ResourceHandle<Mesh>> meshes;
    std::vector<ResourceHandle<Material>> materials;
    std::vector<ResourceHandle<Image>> textures;
};

struct Hierarchy
{
    struct Node
    {
        std::string name {};
        glm::mat4 transform { 1.0f };
        std::optional<uint32_t> meshIndex;
        std::vector<Node> children {};
    };

    std::vector<Node> baseNodes {};
};

namespace CPUResources
{

struct Mesh
{
    struct Primitive
    {
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        Vec3Range boundingBox;

        std::optional<uint32_t> materialIndex;
    };

    std::vector<Primitive> primitives;
};

struct ModelData
{
    struct Image
    {
    };

    struct Material
    {
        using TextureIndex = uint32_t;
        std::optional<TextureIndex> albedoMap;
        glm::vec4 albedoFactor { 0.0f };
        uint32_t albedoUVChannel;

        std::optional<TextureIndex> metallicRoughnessMap;
        float metallicFactor { 0.0f };
        float roughnessFactor { 0.0f };
        std::optional<TextureIndex> metallicRoughnessUVChannel;

        std::optional<TextureIndex> normalMap;
        float normalScale { 0.0f };
        uint32_t normalUVChannel;

        std::optional<TextureIndex> occlusionMap;
        float occlusionStrength { 0.0f };
        uint32_t occlusionUVChannel;

        std::optional<TextureIndex> emissiveMap;
        glm::vec3 emissiveFactor { 0.0f };
        uint32_t emissiveUVChannel;
    };

    Hierarchy hierarchy {};

    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<ImageCreation> textures;
};
};

namespace GPUResources
{
struct Model
{

    Hierarchy hierarchy {};

    std::vector<ResourceHandle<Mesh>> meshes;
    std::vector<ResourceHandle<Material>> materials;
    std::vector<ResourceHandle<Image>> textures;
};
}
