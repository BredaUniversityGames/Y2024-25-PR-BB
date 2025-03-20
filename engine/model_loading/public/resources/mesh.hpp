#pragma once
#include <vector>

#include "math_util.hpp"
#include "vertex.hpp"

enum class MeshType : uint8_t
{
    eSTATIC,
    eSKINNED,
};

template <typename T>
struct CPUMesh
{
    std::vector<T> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialIndex { 0 };

    math::Vec3Range boundingBox;
    float boundingRadius;
};