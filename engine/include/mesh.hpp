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

    glm::vec3 position {};
    glm::vec3 normal {};
    glm::vec4 tangent {};
    glm::vec2 texCoord {};

    Vertex()
    {
    }

    Vertex(glm::vec3 position, glm::vec3 normal, glm::vec4 tangent, glm::vec2 texCoord)
        : position(position)
        , normal(normal)
        , tangent(tangent)
        , texCoord(texCoord)
    {
    }

    static vk::VertexInputBindingDescription GetBindingDescription();

    static std::array<vk::VertexInputAttributeDescription, 4> GetAttributeDescriptions();
};

struct MeshPrimitive
{
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    std::optional<uint32_t> materialIndex;
};

struct Mesh
{
    std::vector<MeshPrimitive> primitives;
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

struct MeshPrimitiveHandle
{
    uint32_t count;
    uint32_t vertexOffset;
    uint32_t indexOffset;

    ResourceHandle<Material> material;
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
    std::vector<ResourceHandle<Material>> materials;
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

    GameObject(const glm::mat4& transform, std::shared_ptr<ModelHandle> model)
        : transform(transform)
        , model(model)
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
        0.5, 0.5, 0.5, 1.0);
};

struct SceneDescription
{
    Camera camera;
    std::vector<std::shared_ptr<ModelHandle>> models;
    std::vector<GameObject> gameObjects;
    DirectionalLight directionalLight;
};