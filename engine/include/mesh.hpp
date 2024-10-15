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

struct StagingMesh
{
    struct Primitive
    {
        std::vector<uint32_t> indices;
        std::vector<Vertex> vertices;
        float boundingRadius;

        std::optional<uint32_t> materialIndex;
    };

    std::vector<StagingMesh::Primitive> primitives;
};

struct Hierarchy
{
    struct Node
    {
        std::string name;
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

// todo : TEMP!!!
struct TransformComponent
{
    glm::mat4 transform;
};

struct NameComponent
{
    std::string name;
};

struct StaticMeshComponent
{
    // todo: replace this with resource handle
    std::shared_ptr<MeshHandle> mesh;
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
    Camera camera {
        .projection = Camera::Projection::eOrthographic,
        .position = glm::vec3 { 7.3f, 1.25f, 4.75f },
        .eulerRotation = glm::vec3 { 0.4f, 3.75f, 0.0f },
        .orthographicSize = 17.0f,
        .nearPlane = -16.0f,
        .farPlane = 32.0f,
        .aspectRatio = 1.0f,
    };

    float shadowBias = 0.002f;

    constexpr static glm::mat4 BIAS_MATRIX {
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0
    };
};

struct SceneDescription
{
    Camera camera;
    std::vector<std::shared_ptr<ModelHandle>> models;
    std::vector<GameObject> gameObjects;
    DirectionalLight directionalLight;
};