#pragma once
#include <optional>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

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