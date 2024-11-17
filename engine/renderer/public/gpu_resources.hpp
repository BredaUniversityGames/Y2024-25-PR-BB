#pragma once

#include "lib/includes_vulkan.hpp"
#include "resource_manager.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vk_mem_alloc.h>

class VulkanContext;

struct SamplerCreation
{
    SamplerCreation& SetGlobalAddressMode(vk::SamplerAddressMode addressMode);

    std::string name {};
    vk::SamplerAddressMode addressModeU { vk::SamplerAddressMode::eRepeat };
    vk::SamplerAddressMode addressModeW { vk::SamplerAddressMode::eRepeat };
    vk::SamplerAddressMode addressModeV { vk::SamplerAddressMode::eRepeat };
    vk::Filter minFilter { vk::Filter::eLinear };
    vk::Filter magFilter { vk::Filter::eLinear };
    bool useMaxAnisotropy { true };
    bool anisotropyEnable { true };
    vk::BorderColor borderColor { vk::BorderColor::eIntOpaqueBlack };
    bool unnormalizedCoordinates { false };
    bool compareEnable { false };
    vk::CompareOp compareOp { vk::CompareOp::eAlways };
    vk::SamplerMipmapMode mipmapMode { vk::SamplerMipmapMode::eLinear };
    float mipLodBias { 0.0f };
    float minLod { 0.0f };
    float maxLod { 1.0f };
};

struct Sampler
{
    Sampler(const SamplerCreation& creation, const std::shared_ptr<VulkanContext>& context);
    ~Sampler();

    Sampler(Sampler&& other) noexcept;
    Sampler& operator=(Sampler&& other) noexcept;

    NON_COPYABLE(Sampler);

    vk::Sampler sampler;

private:
    std::shared_ptr<VulkanContext> _context;
};

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
    ResourceHandle<Sampler> sampler {};

    std::string name;

    ImageCreation& SetData(std::byte* data);
    ImageCreation& SetSize(uint16_t width, uint16_t height, uint16_t depth = 1);
    ImageCreation& SetMips(uint8_t mips);
    ImageCreation& SetFlags(vk::ImageUsageFlags flags);
    ImageCreation& SetFormat(vk::Format format);
    ImageCreation& SetName(std::string_view name);
    ImageCreation& SetType(ImageType type);
    ImageCreation& SetSampler(ResourceHandle<Sampler> sampler);
};

struct Image
{
    Image(const ImageCreation& creation, const std::shared_ptr<VulkanContext>& context);
    ~Image();

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    NON_COPYABLE(Image);

    vk::Image image {};
    std::vector<vk::ImageView> views {};
    vk::ImageView view; // Same as first view in view, or refers to a cubemap view
    VmaAllocation allocation {};
    ResourceHandle<Sampler> sampler {};

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

private:
    std::shared_ptr<VulkanContext> _context;
};

struct MaterialCreation
{
    ResourceHandle<Image> albedoMap = ResourceHandle<Image>::Null();
    glm::vec4 albedoFactor { 0.0f };
    uint32_t albedoUVChannel;

    ResourceHandle<Image> metallicRoughnessMap = ResourceHandle<Image>::Null();
    float metallicFactor { 0.0f };
    float roughnessFactor { 0.0f };
    std::optional<uint32_t> metallicRoughnessUVChannel;

    ResourceHandle<Image> normalMap = ResourceHandle<Image>::Null();
    float normalScale { 0.0f };
    uint32_t normalUVChannel;

    ResourceHandle<Image> occlusionMap = ResourceHandle<Image>::Null();
    float occlusionStrength { 0.0f };
    uint32_t occlusionUVChannel;

    ResourceHandle<Image> emissiveMap = ResourceHandle<Image>::Null();
    glm::vec3 emissiveFactor { 0.0f };
    uint32_t emissiveUVChannel;
};

struct Material
{
    Material(const MaterialCreation& creation, const std::shared_ptr<ResourceManager<Image>>& imageResourceManager);
    ~Material();

    Material(Material&& other) noexcept = default;
    Material& operator=(Material&& other) noexcept = default;

    NON_COPYABLE(Material);

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

private:
    std::shared_ptr<ResourceManager<Image>> _imageResourceManager;
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
    Buffer(const BufferCreation& creation, const std::shared_ptr<VulkanContext>& context);
    ~Buffer();

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    NON_COPYABLE(Buffer);

    vk::Buffer buffer {};
    VmaAllocation allocation {};
    void* mappedPtr = nullptr;
    vk::DeviceSize size {};
    vk::BufferUsageFlags usage {};
    std::string name {};

private:
    std::shared_ptr<VulkanContext> _context;
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
