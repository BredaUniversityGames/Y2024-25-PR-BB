#pragma once

#include <array>
#include "camera.hpp"

struct Vertex
{
    enum Enumeration
    {
        ePOSITION,
        eNORMAL,
        eTANGENT,
        eTEX_COORD
    };

    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec4 tangent{};
    glm::vec2 texCoord{};

    Vertex()
    {
    }

    Vertex(glm::vec3 position, glm::vec3 normal, glm::vec4 tangent, glm::vec2 texCoord)
            : position(position), normal(normal), tangent(tangent), texCoord(texCoord)
    {
    }

    static vk::VertexInputBindingDescription GetBindingDescription();

    static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescriptions();
};

struct MeshPrimitive
{
    vk::PrimitiveTopology topology;

    vk::IndexType indexType;
    std::vector<std::byte> indicesBytes;
    std::vector<Vertex> vertices;

    std::optional<uint32_t> materialIndex;
};

struct Mesh
{
    std::vector<MeshPrimitive> primitives;
};

struct Texture
{
    uint32_t width, height, numChannels;
    std::vector<std::byte> data;
    bool isHDR = false;
    vk::Format format = vk::Format::eR8G8B8A8Unorm;

    vk::Format GetFormat() const
    {
        if(isHDR)
            return vk::Format::eR32G32B32A32Sfloat;

        return format;
    }
};

struct HDR
{
    uint32_t width, height, numChannels;
    std::vector<float> data;

    vk::Format GetFormat() const
    {
        return vk::Format::eR32G32B32A32Sfloat;
    }
};

struct Cubemap
{
    vk::Format format;
    size_t size;
    size_t mipLevels;
    vk::Image image;
    VmaAllocation allocation;
    vk::ImageView view;
    vk::UniqueSampler sampler;
};

struct Material
{
    std::optional<uint32_t> albedoIndex;
    glm::vec4 albedoFactor;
    uint32_t albedoUVChannel;

    std::optional<uint32_t> metallicRoughnessIndex;
    float metallicFactor;
    float roughnessFactor;
    std::optional<uint32_t> metallicRoughnessUVChannel;

    std::optional<uint32_t> normalIndex;
    float normalScale;
    uint32_t normalUVChannel;

    std::optional<uint32_t> occlusionIndex;
    float occlusionStrength;
    uint32_t occlusionUVChannel;

    std::optional<uint32_t> emissiveIndex;
    glm::vec3 emissiveFactor;
    uint32_t emissiveUVChannel;
};

struct TextureHandle
{
    std::string name;
    vk::Image image;
    VmaAllocation imageAllocation;
    vk::ImageView imageView;
    uint32_t width, height;
    vk::Format format;
};

struct MaterialHandle
{
    struct alignas(16) MaterialInfo
    {
        glm::vec4 albedoFactor{ 0.0f };

        float metallicFactor{ 0.0f };
        float roughnessFactor{ 0.0f };
        float normalScale{ 0.0f };
        float occlusionStrength{ 0.0f };

        glm::vec3 emissiveFactor{ 0.0f };
        int32_t useEmissiveMap{ false };

        int32_t useAlbedoMap{ false };
        int32_t useMRMap{ false };
        int32_t useNormalMap{ false };
        int32_t useOcclusionMap{ false };

        int32_t albedoMapIndex;
        int32_t mrMapIndex;
        int32_t normalMapIndex;
        int32_t occlusionMapIndex;
        int32_t emissiveMapIndex;
    };

    const static uint32_t TEXTURE_COUNT = 5;

    vk::DescriptorSet descriptorSet;
    vk::Buffer materialUniformBuffer;
    VmaAllocation materialUniformAllocation;

    std::array<ResourceHandle<Image>, TEXTURE_COUNT> textures;

    static std::array<vk::DescriptorSetLayoutBinding, 1> GetLayoutBindings()
    {
        std::array<vk::DescriptorSetLayoutBinding, 1> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = vk::ShaderStageFlagBits::eFragment;

        return bindings;
    }
};

struct MeshPrimitiveHandle
{
    vk::PrimitiveTopology topology;
    vk::IndexType indexType;
    uint32_t indexCount;

    vk::Buffer vertexBuffer;
    vk::Buffer indexBuffer;
    VmaAllocation vertexBufferAllocation;
    VmaAllocation indexBufferAllocation;

    std::shared_ptr<MaterialHandle> material;
};

struct MeshHandle
{
    std::vector<MeshPrimitiveHandle> primitives;
};

struct Hierarchy
{
    struct Node
    {
        glm::mat4 transform;
        std::shared_ptr<MeshHandle> mesh;
    };

    std::vector<Node> allNodes;
};

struct ModelHandle
{
    std::vector<std::shared_ptr<MeshHandle>> meshes;
    std::vector<std::shared_ptr<MaterialHandle>> materials;
    std::vector<ResourceHandle<Image>> textures;

    Hierarchy hierarchy;
};

struct GameObject
{
    glm::mat4 transform;
    std::shared_ptr<ModelHandle> model;

    GameObject()
    {
    }

    GameObject(const glm::mat4 &transform, std::shared_ptr<ModelHandle> model)
            : transform(transform), model(model)
    {
    }
};

struct DirectionalLight
{
    glm::vec3 targetPos = glm::vec3(0.0f, 1.5f, -0.25f);
    glm::vec3 lightDir = glm::vec3(0.2f, -0.15f, 0.15f);
    float sceneDistance = 1.0f;
    float orthoSize = 17.0f;
    float farPlane = 32.0f;
    float nearPlane = -16.0f;
    float shadowBias = 0.002f;

    const glm::mat4 biasMatrix = glm::mat4(
            0.5, 0.0, 0.0, 0.0,
            0.0, 0.5, 0.0, 0.0,
            0.0, 0.0, 0.5, 0.0,
            0.5, 0.5, 0.5, 1.0
    );
};

struct SceneDescription
{
    Camera camera;
    std::vector<std::shared_ptr<ModelHandle>> models;
    std::vector<GameObject> gameObjects;
    DirectionalLight directionalLight;
};