#pragma once

#include "animation.hpp"
#include "components/animation_channel_component.hpp"
#include "math_util.hpp"
#include "vertex.hpp"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>

struct Hierarchy
{
    struct Joint
    {
        glm::mat4 inverseBind;
        uint32_t index;
    };

    struct Node
    {
        Node() = default;
        Node(std::string_view name, const glm::mat4& transform, std::optional<std::pair<MeshType, uint32_t>> meshIndex = std::nullopt)
            : name(name)
            , transform(transform)
            , meshIndex(meshIndex)
        {
        }

        std::string name {};
        glm::mat4 transform { 1.0f };
        std::optional<std::pair<MeshType, uint32_t>> meshIndex = std::nullopt;
        std::vector<uint32_t> children {};

        std::unordered_map<uint32_t, TransformAnimationSpline> animationSplines {};
        std::optional<Joint> joint {};
        std::optional<uint32_t> skeletonNode {};
    };

    uint32_t root {};
    std::optional<uint32_t> skeletonRoot = std::nullopt;
    std::vector<Node> nodes {};
};

template <typename T>
struct CPUMesh
{
    std::vector<T> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialIndex { 0 };

    math::Vec3Range boundingBox;
    float boundingRadius;
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
        float normalScale { 1.0f };
        uint32_t normalUVChannel;

        std::optional<TextureIndex> occlusionMap;
        float occlusionStrength { 0.0f };
        uint32_t occlusionUVChannel;

        std::optional<TextureIndex> emissiveMap;
        glm::vec3 emissiveFactor { 0.0f };
        uint32_t emissiveUVChannel;
    };

    Hierarchy hierarchy {};

    std::vector<CPUMesh<Vertex>> meshes {};
    std::vector<CPUMesh<SkinnedVertex>> skinnedMeshes {};

    std::vector<CPUMaterial> materials {};
    std::vector<CPUImage> textures {};

    std::vector<Animation> animations {};
};

struct GPUModel
{
    std::vector<ResourceHandle<GPUMesh>> staticMeshes;
    std::vector<ResourceHandle<GPUMesh>> skinnedMeshes;
    std::vector<ResourceHandle<GPUMaterial>> materials;
    std::vector<ResourceHandle<GPUImage>> textures;
};
