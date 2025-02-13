#pragma once
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "resources/animation.hpp"
#include "resources/image.hpp"
#include "resources/mesh_data.hpp"

struct Hierarchy
{
    struct Joint
    {
        uint32_t index {};
        glm::mat4 inverseBind {};
    };

    struct Node
    {
        std::string name {};
        glm::mat4 transform {};
        std::vector<uint32_t> children {};

        std::optional<std::pair<MeshType, uint32_t>> meshTypeAndIndex {};

        std::optional<Joint> joint {};
        std::unordered_map<uint32_t, TransformAnimationSpline> animationSplines {};
        std::optional<uint32_t> skeletonNode {};
    };

    uint32_t root {};
    std::optional<uint32_t> skeletonRoot = std::nullopt;
    std::vector<Node> nodes {};
};

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

struct CPUModel
{
    Hierarchy hierarchy {};

    std::vector<CPUMesh<Vertex>> meshes {};
    std::vector<CPUMesh<SkinnedVertex>> skinnedMeshes {};

    std::vector<CPUMaterial> materials {};
    std::vector<CPUImage> images {};

    std::vector<Animation> animations {};
};