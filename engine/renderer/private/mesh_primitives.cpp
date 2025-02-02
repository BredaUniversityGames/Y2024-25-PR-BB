#include "mesh_primitives.hpp"
#include <glm/gtc/constants.hpp>

void AddTriangle(std::vector<uint32_t>& indices, std::array<uint32_t, 3> triangle)
{
    indices.emplace_back(triangle[0]);
    indices.emplace_back(triangle[1]);
    indices.emplace_back(triangle[2]);
}

CPUMesh<Vertex> GenerateUVSphere(uint32_t slices, uint32_t stacks, float radius)
{
    CPUMesh<Vertex> mesh;

    uint32_t totalVertices = 2 + (stacks - 1) * slices;

    mesh.vertices.reserve(totalVertices);

    // TODO: Consider this based on the total amount of indices instead.
    mesh.materialIndex = 0;

    for (uint32_t i = 0; i <= stacks; ++i)
    {
        float theta = i * glm::pi<float>() / stacks;

        for (uint32_t j = 0; j <= slices; ++j)
        {
            float phi = j * (glm::pi<float>() * 2.0f) / slices;
            glm::vec3 point {
                cosf(phi) * sinf(theta),
                cosf(theta),
                sinf(phi) * sinf(theta)
            };

            float u = static_cast<float>(j) / slices;
            float v = static_cast<float>(i) / stacks;
            glm::vec2 texCoords { u, v };
            glm::vec3 position { point * radius };

            mesh.vertices.emplace_back(position, point, glm::vec4 {}, texCoords);
        }
    }

    using Triangle = std::array<uint32_t, 3>;
    for (uint32_t i = 0; i < stacks; ++i)
    {
        for (uint32_t j = 0; j < slices; ++j)
        {
            uint32_t first = i * (slices + 1) + j;
            uint32_t second = first + slices + 1;

            AddTriangle(mesh.indices, Triangle { first, second, first + 1 });
            AddTriangle(mesh.indices, Triangle { second, second + 1, first + 1 });
        }
    }

    mesh.boundingRadius = radius;
    mesh.boundingBox = { glm::vec3 { -radius }, glm::vec3 { radius } };
    return mesh;
}
