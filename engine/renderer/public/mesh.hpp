#pragma once

#include "camera.hpp"
#include <array>
#include <memory>

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
        float boundingRadius;

        std::optional<uint32_t> materialIndex;
    };

    std::vector<StagingMesh::Primitive> primitives;
};

struct StaticMeshComponent
{
    ResourceHandle<Mesh> mesh;
};

struct SceneDescription
{
    Camera camera;
};