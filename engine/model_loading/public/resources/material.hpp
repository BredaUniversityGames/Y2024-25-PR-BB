#pragma once
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>

#include "resources/image.hpp"

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

struct MaterialCreation
{
    ResourceHandle<GPUImage> albedoMap = ResourceHandle<GPUImage>::Null();
    glm::vec4 albedoFactor { 0.0f };
    uint32_t albedoUVChannel;

    ResourceHandle<GPUImage> metallicRoughnessMap = ResourceHandle<GPUImage>::Null();
    float metallicFactor { 0.0f };
    float roughnessFactor { 0.0f };
    std::optional<uint32_t> metallicRoughnessUVChannel;

    ResourceHandle<GPUImage> normalMap = ResourceHandle<GPUImage>::Null();
    float normalScale { 0.0f };
    uint32_t normalUVChannel;

    ResourceHandle<GPUImage> occlusionMap = ResourceHandle<GPUImage>::Null();
    float occlusionStrength { 0.0f };
    uint32_t occlusionUVChannel;

    ResourceHandle<GPUImage> emissiveMap = ResourceHandle<GPUImage>::Null();
    glm::vec3 emissiveFactor { 0.0f };
    uint32_t emissiveUVChannel;
};

struct GPUMaterial
{
    GPUMaterial() = default;
    GPUMaterial(const MaterialCreation& creation, const std::shared_ptr<ResourceManager<GPUImage>>& imageResourceManager);
    ~GPUMaterial();

    GPUMaterial(GPUMaterial&& other) noexcept = default;
    GPUMaterial& operator=(GPUMaterial&& other) noexcept = default;

    NON_COPYABLE(GPUMaterial);

    // Info that gets send to the gpu
    struct alignas(16) GPUInfo
    {
        glm::vec4 albedoFactor { 0.0f };

        float metallicFactor { 0.0f };
        float roughnessFactor { 0.0f };
        float normalScale { 1.0f };
        float occlusionStrength { 0.0f };

        glm::vec3 emissiveFactor { 0.0f };
        int32_t useEmissiveMap { false };

        int32_t useAlbedoMap { false };
        int32_t useMRMap { false };
        int32_t useNormalMap { false };
        int32_t useOcclusionMap { false };

        uint32_t albedoMapIndex;
        uint32_t mrMapIndex;
        uint32_t normalMapIndex;
        uint32_t occlusionMapIndex;

        uint32_t emissiveMapIndex;
    } gpuInfo;

    ResourceHandle<GPUImage> albedoMap;
    ResourceHandle<GPUImage> mrMap;
    ResourceHandle<GPUImage> normalMap;
    ResourceHandle<GPUImage> occlusionMap;
    ResourceHandle<GPUImage> emissiveMap;

private:
    std::shared_ptr<ResourceManager<GPUImage>> _imageResourceManager;
};