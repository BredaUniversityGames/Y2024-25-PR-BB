#pragma once
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"
#include "resource_manager.hpp"

enum class ImageType
{
    e2D,
    e2DArray,
    eCubeMap,
    eShadowMap
};

struct ImageCreation
{
    std::byte* initialData { nullptr };
    uint16_t width { 1 };
    uint16_t height { 1 };
    uint16_t depth { 1 };
    uint16_t layers { 1 };
    uint8_t mips { 1 };
    vk::ImageUsageFlags flags { 0 };
    bool isHDR;

    vk::Format format { vk::Format::eUndefined };
    ImageType type { ImageType::e2D };
    vk::Sampler sampler { nullptr };

    std::string name;

    ImageCreation& SetData(std::byte* data);
    ImageCreation& SetSize(uint16_t width, uint16_t height, uint16_t depth = 1);
    ImageCreation& SetMips(uint8_t mips);
    ImageCreation& SetFlags(vk::ImageUsageFlags flags);
    ImageCreation& SetFormat(vk::Format format);
    ImageCreation& SetName(std::string_view name);
    ImageCreation& SetType(ImageType type);
    ImageCreation& SetSampler(vk::Sampler sampler);
};

struct Image
{
    vk::Image image {};
    std::vector<vk::ImageView> views {};
    vk::ImageView view; // Same as first view in view, or refers to a cubemap view
    VmaAllocation allocation {};
    vk::Sampler sampler { nullptr };

    uint16_t width { 1 };
    uint16_t height { 1 };
    uint16_t depth { 1 };
    uint16_t layers { 1 };
    uint8_t mips { 1 };
    vk::ImageUsageFlags flags { 0 };
    bool isHDR;
    ImageType type;

    vk::Format format { vk::Format::eUndefined };
    vk::ImageType vkType { vk::ImageType::e2D };

    std::string name;
};

struct MaterialCreation
{
    ResourceHandle<Image> albedoMap = ResourceHandle<Image>::Invalid();
    glm::vec4 albedoFactor { 0.0f };
    uint32_t albedoUVChannel;

    ResourceHandle<Image> metallicRoughnessMap = ResourceHandle<Image>::Invalid();
    float metallicFactor { 0.0f };
    float roughnessFactor { 0.0f };
    std::optional<uint32_t> metallicRoughnessUVChannel;

    ResourceHandle<Image> normalMap = ResourceHandle<Image>::Invalid();
    float normalScale { 0.0f };
    uint32_t normalUVChannel;

    ResourceHandle<Image> occlusionMap = ResourceHandle<Image>::Invalid();
    float occlusionStrength { 0.0f };
    uint32_t occlusionUVChannel;

    ResourceHandle<Image> emissiveMap = ResourceHandle<Image>::Invalid();
    glm::vec3 emissiveFactor { 0.0f };
    uint32_t emissiveUVChannel;
};

struct Material
{
    struct alignas(16) GPUInfo
    {
        glm::vec4 albedoFactor { 0.0f };

        float metallicFactor { 0.0f };
        float roughnessFactor { 0.0f };
        float normalScale { 0.0f };
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

    ResourceHandle<Image> albedoMap;
    ResourceHandle<Image> mrMap;
    ResourceHandle<Image> normalMap;
    ResourceHandle<Image> occlusionMap;
    ResourceHandle<Image> emissiveMap;
};

struct BufferCreation
{
    vk::DeviceSize size {};
    vk::BufferUsageFlags usage {};
    bool isMappable = true;
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY;
    std::string name {};

    BufferCreation& SetSize(vk::DeviceSize size);
    BufferCreation& SetUsageFlags(vk::BufferUsageFlags usage);
    BufferCreation& SetIsMappable(bool isMappable);
    BufferCreation& SetMemoryUsage(VmaMemoryUsage memoryUsage);
    BufferCreation& SetName(std::string_view name);
};

struct Buffer
{
    vk::Buffer buffer {};
    VmaAllocation allocation {};
    void* mappedPtr = nullptr;
    vk::DeviceSize size {};
    vk::BufferUsageFlags usage {};
};

struct Mesh
{
    struct Primitive
    {
        uint32_t count;
        uint32_t vertexOffset;
        uint32_t indexOffset;
        float boundingRadius;

        ResourceHandle<Material> material;
    };

    std::vector<Primitive> primitives;
};

struct alignas(16) GPUCamera
{
    glm::mat4 VP;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 skydomeMVP; // TODO: remove this

    glm::vec3 cameraPosition;
    bool distanceCullingEnabled;
    float frustum[4];
    float zNear;
    float zFar;
    bool cullingEnabled;
    int32_t projectionType;

    glm::vec2 _padding {};
};
