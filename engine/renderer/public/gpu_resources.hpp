#pragma once

#include "commands/single_time_commands.hpp"
#include "math_util.hpp"
#include "resource_manager.hpp"
#include "vulkan_include.hpp"

#include "resources/buffer.hpp"
#include "resources/image.hpp"
#include "resources/sampler.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vk_mem_alloc.h>

class VulkanContext;

enum class MeshType : uint8_t
{
    eSTATIC,
    eSKINNED,
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

struct GPUMesh
{
    uint32_t count { 0 };
    uint32_t vertexOffset { 0 };
    uint32_t indexOffset { 0 };
    float boundingRadius;
    math::Vec3Range boundingBox;

    MeshType type;
    ResourceHandle<GPUMaterial> material;
};

struct alignas(16) GPUCamera
{
    glm::mat4 VP;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 skydomeMVP; // TODO: remove this
    glm::mat4 inverseView;
    glm::mat4 inverseProj;
    glm::mat4 inverseVP;

    glm::vec3 cameraPosition;
    bool distanceCullingEnabled;
    float frustum[4];
    float zNear;
    float zFar;
    bool cullingEnabled;
    int32_t projectionType;

    glm::vec2 _padding {};
};
