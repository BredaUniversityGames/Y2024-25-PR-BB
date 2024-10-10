#pragma once

#include <array>
#include "camera.hpp"

struct LineVertex
{
    glm::vec3 position;
    static vk::VertexInputBindingDescription GetBindingDescription()
    {
        vk::VertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(LineVertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 1> GetAttributeDescriptions()
    {
        std::array<vk::VertexInputAttributeDescription, 1> attributeDescriptions {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0; // Matches location in shader
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(LineVertex, position);
        return attributeDescriptions;
    }
};

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

struct StagingMesh
{
    struct Primitive
    {
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;

        std::optional<uint32_t> materialIndex;
    };

    std::vector<StagingMesh::Primitive> primitives;
};

struct Hierarchy
{
    struct Node
    {
        glm::mat4 transform;
        ResourceHandle<Mesh> mesh;
    };

    std::vector<Node> allNodes;
};

struct ModelHandle
{
    std::vector<ResourceHandle<Mesh>> meshes;
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