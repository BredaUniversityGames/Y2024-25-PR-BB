#include "mesh.hpp"

vk::VertexInputBindingDescription Vertex::GetBindingDescription()
{
    vk::VertexInputBindingDescription bindingDesc;
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = vk::VertexInputRate::eVertex;

    return bindingDesc;
}

std::array<vk::VertexInputAttributeDescription, 4> Vertex::GetAttributeDescriptions()
{
    std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions {};
    attributeDescriptions[ePOSITION].binding = 0;
    attributeDescriptions[ePOSITION].location = 0;
    attributeDescriptions[ePOSITION].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[ePOSITION].offset = offsetof(Vertex, position);

    attributeDescriptions[eNORMAL].binding = 0;
    attributeDescriptions[eNORMAL].location = 1;
    attributeDescriptions[eNORMAL].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[eNORMAL].offset = offsetof(Vertex, normal);

    attributeDescriptions[eTANGENT].binding = 0;
    attributeDescriptions[eTANGENT].location = 2;
    attributeDescriptions[eTANGENT].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[eTANGENT].offset = offsetof(Vertex, tangent);

    attributeDescriptions[eTEX_COORD].binding = 0;
    attributeDescriptions[eTEX_COORD].location = 3;
    attributeDescriptions[eTEX_COORD].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[eTEX_COORD].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

vk::VertexInputBindingDescription SkinnedVertex::GetBindingDescription()
{
    vk::VertexInputBindingDescription bindingDesc;
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(SkinnedVertex);
    bindingDesc.inputRate = vk::VertexInputRate::eVertex;

    return bindingDesc;
}

std::array<vk::VertexInputAttributeDescription, 6> SkinnedVertex::GetAttributeDescriptions()
{
    std::array<vk::VertexInputAttributeDescription, 6> attributeDescriptions {};
    attributeDescriptions[ePOSITION].binding = 0;
    attributeDescriptions[ePOSITION].location = 0;
    attributeDescriptions[ePOSITION].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[ePOSITION].offset = offsetof(SkinnedVertex, position);

    attributeDescriptions[eNORMAL].binding = 0;
    attributeDescriptions[eNORMAL].location = 1;
    attributeDescriptions[eNORMAL].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[eNORMAL].offset = offsetof(SkinnedVertex, normal);

    attributeDescriptions[eTANGENT].binding = 0;
    attributeDescriptions[eTANGENT].location = 2;
    attributeDescriptions[eTANGENT].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[eTANGENT].offset = offsetof(SkinnedVertex, tangent);

    attributeDescriptions[eTEX_COORD].binding = 0;
    attributeDescriptions[eTEX_COORD].location = 3;
    attributeDescriptions[eTEX_COORD].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[eTEX_COORD].offset = offsetof(SkinnedVertex, texCoord);

    attributeDescriptions[eJOINTS].binding = 0;
    attributeDescriptions[eJOINTS].location = 4;
    attributeDescriptions[eJOINTS].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[eJOINTS].offset = offsetof(SkinnedVertex, joints);

    attributeDescriptions[eWEIGHTS].binding = 0;
    attributeDescriptions[eWEIGHTS].location = 5;
    attributeDescriptions[eWEIGHTS].format = vk::Format::eR32G32B32A32Sfloat;
    attributeDescriptions[eWEIGHTS].offset = offsetof(SkinnedVertex, weights);

    return attributeDescriptions;
}
