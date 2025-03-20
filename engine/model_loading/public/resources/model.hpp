#pragma once
#include "animation.hpp"
#include "gpu_resources.hpp"
#include "resources/hierarchy.hpp"
#include "resources/material.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

struct CPUModel
{
    std::string name {};
    Hierarchy hierarchy {};

    std::vector<CPUMesh<Vertex>> meshes {};
    std::vector<CPUMesh<SkinnedVertex>> skinnedMeshes {};

    std::vector<CPUMaterial> materials {};
    std::vector<CPUImage> textures {};
    std::vector<Animation> animations {};
    std::vector<JPH::ShapeRefC> colliders {};
};