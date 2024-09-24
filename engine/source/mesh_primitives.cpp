#include "mesh_primitives.hpp"
#include <concepts>

template<typename T>
concept indexType = std::same_as<T, uint16_t> || std::same_as<T, uint32_t>;

template <indexType T>
void AddTriangle(std::vector<std::byte>& indicesBytes, std::array<T, 3> triangle)
{
    size_t sizeBefore = indicesBytes.size();
    indicesBytes.resize(indicesBytes.size() + sizeof(triangle));
    std::memcpy(indicesBytes.data() + sizeBefore, triangle.data(), sizeof(triangle));
}

MeshPrimitive GenerateUVSphere(uint32_t slices, uint32_t stacks, float radius)
{
    MeshPrimitive primitive;

    uint32_t totalVertices = 2 + (stacks - 1) * slices;

    primitive.vertices.reserve(totalVertices);

    // TODO: Consider this based on the total amount of indices instead.
    primitive.indexType = vk::IndexType::eUint32;
    primitive.topology = vk::PrimitiveTopology::eTriangleList;
    primitive.materialIndex = std::nullopt;

    for(uint32_t i = 0; i <= stacks; ++i)
    {
        float theta = i * glm::pi<float>() / stacks;

        for(uint32_t j = 0; j <= slices; ++j)
        {
            float phi = j * (glm::pi<float>() * 2.0f) / slices;
            glm::vec3 point{
                cosf(phi) * sinf(theta),
                cosf(theta),
                sinf(phi) * sinf(theta)
            };

            float u = static_cast<float>(j) / slices;
            float v = static_cast<float>(i) / stacks;
            glm::vec2 texCoords{ u, v };
            glm::vec3 position{ point * radius };

            primitive.vertices.emplace_back(position, point, glm::vec4{}, glm::vec3{}, texCoords);
        }
    }

    using Triangle = std::array<uint32_t, 3>;
    for(uint32_t i = 0; i < stacks; ++i)
    {
        for(uint32_t j = 0; j < slices; ++j)
        {
            uint32_t first = i * (slices + 1) + j;
            uint32_t second = first + slices + 1;

            AddTriangle(primitive.indicesBytes, Triangle{ first, second, first + 1 });
            AddTriangle(primitive.indicesBytes, Triangle{ second, second + 1, first + 1 });
        }
    }

    return primitive;
}
