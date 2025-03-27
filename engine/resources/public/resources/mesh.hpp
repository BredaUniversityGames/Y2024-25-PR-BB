#pragma once
#include <vector>

#include "math_util.hpp"
#include "resources/material.hpp"
#include "resources/mesh.hpp"

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

struct GPUMesh
{
    uint32_t count { 0 };
    uint32_t vertexOffset { 0 };
    uint32_t indexOffset { 0 };
    float boundingRadius;
    math::Vec3Range boundingBox;

    MeshType type;
    ResourceHandle<GPUMaterial> material;
};