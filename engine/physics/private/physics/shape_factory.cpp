#include "physics/shape_factory.hpp"
#include "physics/jolt_to_glm.hpp"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

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

JPH::ShapeRefC ShapeFactory::MakeMeshHullShape(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
{
    JPH::Array<JPH::Float3> joltVertices;
    joltVertices.reserve(vertices.size());

    for (auto& v : vertices)
        joltVertices.emplace_back(v.x, v.y, v.z);

    auto triangleCount = indices.size() / 3;
    JPH::Array<JPH::IndexedTriangle> joltTriangles;
    joltTriangles.reserve(triangleCount);

    for (size_t i = 0; i < triangleCount; ++i)
    {
        joltTriangles.emplace_back(JPH::IndexedTriangle(indices[3 * i], indices[3 * i + 1], indices[3 * i + 2]));
    }

    JPH::MeshShapeSettings creation { std::move(joltVertices), std::move(joltTriangles) };

    JPH::Shape::ShapeResult result {};
    JPH::ShapeRefC shape = new JPH::MeshShape(creation, result);

    if (result.HasError())
        return nullptr;

    return shape;
}