#pragma once

#include "animation.hpp"
#include "mesh.hpp"
#include <glm/mat4x4.hpp>
#include <vector>

struct Hierarchy
{
    struct Joint
    {
        bool isSkeletonRoot;
        glm::mat4 inverseBind;
    };

    struct Node
    {
        std::string name {};
        glm::mat4 transform { 1.0f };
        std::optional<uint32_t> meshIndex = std::nullopt;
        std::vector<Node> children {};

        std::optional<AnimationChannel> animationChannel {};
        std::optional<Joint> joint;
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

    std::optional<Animation> animation { std::nullopt };
};

struct GPUModel
{
    std::vector<ResourceHandle<GPUMesh>> meshes;
    std::vector<ResourceHandle<GPUMaterial>> materials;
    std::vector<ResourceHandle<GPUImage>> textures;
};
