#pragma once

#include "mesh.hpp"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

struct Hierarchy
{
    struct Node
    {
        std::string name {};
        glm::mat4 transform { 1.0f };
        std::optional<uint32_t> meshIndex = std::nullopt;
        std::vector<Node> children {};
    };

    std::vector<Node> baseNodes {};
};

struct CPUMesh
{
    struct Primitive
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t materialIndex { 0 };

        Vec3Range boundingBox;
        float boundingRadius;
    };

    Vec3Range GetMeshBounds() const
    {
        glm::vec3 min { std::numeric_limits<float>::max() };
        glm::vec3 max { std::numeric_limits<float>::min() };
        for (const auto& primitive : primitives)
        {
            min.x = glm::min(min.x, primitive.boundingBox.min.x);
            min.y = glm::min(min.y, primitive.boundingBox.min.y);
            min.z = glm::min(min.z, primitive.boundingBox.min.z);
            max.x = glm::max(max.x, primitive.boundingBox.max.x);
            max.y = glm::max(max.y, primitive.boundingBox.max.y);
            max.z = glm::max(max.z, primitive.boundingBox.max.z);
        }
        return Vec3Range(min, max);
    }

    std::vector<Primitive> primitives;
};

struct CPUModel
{

    // For now this is only meant to be used in combination with an owning CPUModel.
    struct CPUMaterial
    {
        // Texture indices corrospond to the index in the array of textures located in the owning CPUModel
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

    std::vector<CPUMesh> meshes;
    std::vector<CPUMaterial> materials;
    std::vector<CPUImage> textures;
};

struct GPUModel
{
    std::vector<ResourceHandle<GPUMesh>> meshes;
    std::vector<ResourceHandle<GPUMaterial>> materials;
    std::vector<ResourceHandle<GPUImage>> textures;
};
