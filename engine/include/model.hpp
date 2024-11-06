#pragma once

#include <vector>
#include <glm/mat4x4.hpp>
#include "mesh.hpp"

struct ModelResources
{
    std::vector<ResourceHandle<Mesh>> meshes;
    std::vector<ResourceHandle<Material>> materials;
    std::vector<ResourceHandle<Image>> textures;
};

struct Hierarchy
{
    struct Node
    {
        std::string name {};
        glm::mat4 transform { 1.0f };
        ResourceHandle<Mesh> mesh {};
        std::vector<Node> children {};
    };

    std::vector<Node> baseNodes {};
};

struct Model
{
    ModelResources resources;
    Hierarchy hierarchy;
};
