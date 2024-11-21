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

struct CPUModelData
{
    struct CPUMaterialData
    {
        using TextureDataIndex = uint32_t;

        std::optional<TextureDataIndex> albedoMap;
        glm::vec4 albedoFactor { 0.0f };
        uint32_t albedoUVChannel;

        std::optional<TextureDataIndex> metallicRoughnessMap;
        float metallicFactor { 0.0f };
        float roughnessFactor { 0.0f };
        std::optional<uint32_t> metallicRoughnessUVChannel;

        std::optional<TextureDataIndex> normalMap;
        float normalScale { 0.0f };
        uint32_t normalUVChannel;

        std::optional<TextureDataIndex> occlusionMap;
        float occlusionStrength { 0.0f };
        uint32_t occlusionUVChannel;

        std::optional<TextureDataIndex> emissiveMap;
        glm::vec3 emissiveFactor { 0.0f };
        uint32_t emissiveUVChannel;
    };

    Hierarchy hierarchy {};

    std::vector<CPUMesh> meshes;
    std::vector<CPUMaterialData> materials;
    std::vector<ImageCreation> textures;
};
