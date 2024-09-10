#include "mesh.hpp"

vk::VertexInputBindingDescription Vertex::GetBindingDescription()
{
    vk::VertexInputBindingDescription bindingDesc;
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = vk::VertexInputRate::eVertex;

    return bindingDesc;
}

std::array<vk::VertexInputAttributeDescription, 5> Vertex::GetAttributeDescriptions()
{
    std::array<vk::VertexInputAttributeDescription, 5> attributeDescriptions{};
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

    attributeDescriptions[eCOLOR].binding = 0;
    attributeDescriptions[eCOLOR].location = 3;
    attributeDescriptions[eCOLOR].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[eCOLOR].offset = offsetof(Vertex, color);

    attributeDescriptions[eTEX_COORD].binding = 0;
    attributeDescriptions[eTEX_COORD].location = 4;
    attributeDescriptions[eTEX_COORD].format = vk::Format::eR32G32Sfloat;
    attributeDescriptions[eTEX_COORD].offset = offsetof(Vertex, texCoord);


    return attributeDescriptions;
}


