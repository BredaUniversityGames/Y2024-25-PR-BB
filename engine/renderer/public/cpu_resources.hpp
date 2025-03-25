#pragma once

#include "animation.hpp"
#include "components/animation_channel_component.hpp"
#include "math_util.hpp"
#include "vertex.hpp"

#include "resources/hierarchy.hpp"
#include "resources/mesh.hpp"
#include "resources/model.hpp"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include <vector>

struct GPUModel
{
    std::vector<ResourceHandle<GPUMesh>> staticMeshes;
    std::vector<ResourceHandle<GPUMesh>> skinnedMeshes;
    std::vector<ResourceHandle<GPUMaterial>> materials;
    std::vector<ResourceHandle<GPUImage>> textures;
};
