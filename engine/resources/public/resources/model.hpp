#pragma once
#include "animation.hpp"
#include "resource_manager.hpp"
#include "resources/hierarchy.hpp"
#include "resources/image.hpp"
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

struct GPUModel
{
    std::vector<ResourceHandle<GPUMesh>> staticMeshes;
    std::vector<ResourceHandle<GPUMesh>> skinnedMeshes;
    std::vector<ResourceHandle<GPUMaterial>> materials;
    std::vector<ResourceHandle<GPUImage>> textures;
};
