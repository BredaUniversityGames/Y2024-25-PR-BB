#include "physics/shape_factory.hpp"
#include "physics/jolt_to_glm.hpp"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "log.hpp"

JPH::ShapeRefC ShapeFactory::MakeBoxShape(const glm::vec3& size)
{
    return new JPH::BoxShape(ToJoltVec3(size * 0.5f));
}

JPH::ShapeRefC ShapeFactory::MakeSphereShape(float radius)
{
    return new JPH::SphereShape(radius);
}

JPH::ShapeRefC ShapeFactory::MakeCapsuleShape(float cylinderHeight, float radius)
{
    return new JPH::CapsuleShape(cylinderHeight * 0.5f, radius);
}

JPH::ShapeRefC ShapeFactory::MakeConvexHullShape(const std::vector<glm::vec3>& vertices)
{
    JPH::Array<JPH::Vec3> joltVertices;
    joltVertices.reserve(vertices.size());

    for (auto& v : vertices)
        joltVertices.emplace_back(ToJoltVec3(v));

    JPH::ConvexHullShapeSettings creation { std::move(joltVertices) };

    JPH::Shape::ShapeResult result {};
    JPH::ShapeRefC shape = new JPH::ConvexHullShape(creation, result);

    if (result.HasError())
        return nullptr;

    return shape;
}

bool isTriangleDegenerate(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    auto cross = glm::cross(v1 - v0, v2 - v0);
    auto lengthSquared = cross.x * cross.x + cross.y * cross.y + cross.z * cross.z;

    if (lengthSquared <= 1e-9f)
    {
        bblog::info("{}", lengthSquared);
    }

    return lengthSquared <= 1e-9f;
}

JPH::ShapeRefC ShapeFactory::MakeMeshHullShape(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
{
    JPH::Array<JPH::Float3> joltVertices;
    joltVertices.reserve(vertices.size());

    static size_t vert_count = 0;
    static size_t degenerate_count = 0;

    vert_count += vertices.size();

    for (auto& v : vertices)
        joltVertices.emplace_back(v.x, v.y, v.z);

    auto triangleCount = indices.size() / 3;
    JPH::Array<JPH::IndexedTriangle> joltTriangles;
    joltTriangles.reserve(triangleCount);

    for (size_t i = 0; i < triangleCount; ++i)
    {
        if (isTriangleDegenerate(vertices[indices[3 * i]], vertices[indices[3 * i + 1]], vertices[indices[3 * i + 2]]))
        {
            degenerate_count++;
        }
        joltTriangles.emplace_back(JPH::IndexedTriangle(indices[3 * i], indices[3 * i + 1], indices[3 * i + 2]));
    }

    JPH::MeshShapeSettings creation { std::move(joltVertices), std::move(joltTriangles) };

    JPH::Shape::ShapeResult result {};
    JPH::ShapeRefC shape = new JPH::MeshShape(creation, result);

    if (result.HasError())
        return nullptr;

    bblog::info("Physics mesh vertex count: {}", vert_count);
    bblog::info("Physics degenerate triangle count: {}", degenerate_count);

    return shape;
}