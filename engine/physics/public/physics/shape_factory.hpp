#pragma once

#include "physics/jolt_to_glm.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

// Collection of functions to easily create shapes
// Really only a class so that it is easily reflected into wren
struct ShapeFactory
{
    ShapeFactory() = delete;

    static JPH::ShapeRefC MakeBoxShape(const glm::vec3& size);
    static JPH::ShapeRefC MakeSphereShape(float radius);
    static JPH::ShapeRefC MakeCapsuleShape(float cylinderHeight, float radius);
    static JPH::ShapeRefC MakeConvexHullShape(const std::vector<glm::vec3>& vertices);
    static JPH::ShapeRefC MakeMeshHullShape(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
};