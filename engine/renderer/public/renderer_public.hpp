#pragma once

#include <glm/glm.hpp>

#include "camera_public.hpp"
#include "resource_handle.hpp"

struct DirectionalLight
{
    Camera camera {
        .projection = Camera::Projection::eOrthographic,
        .position = glm::vec3 { 7.3f, 1.25f, 4.75f },
        .eulerRotation = glm::vec3 { 0.4f, 3.75f, 0.0f },
        .orthographicSize = 17.0f,
        .nearPlane = -16.0f,
        .farPlane = 32.0f,
        .aspectRatio = 1.0f,
    };

    float shadowBias = 0.002f;

    constexpr static glm::mat4 BIAS_MATRIX {
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0
    };
};

struct Mesh;
struct Material;
struct Image;

struct Hierarchy
{
    struct Node
    {
        glm::mat4 transform;
        ResourceHandle<Mesh> mesh;
    };

    std::vector<Node> allNodes;
};

struct ModelHandle
{
    std::vector<ResourceHandle<Mesh>> meshes;
    std::vector<ResourceHandle<Material>> materials;
    std::vector<ResourceHandle<Image>> textures;

    Hierarchy hierarchy;
};

struct GameObject
{
    glm::mat4 transform;
    std::shared_ptr<ModelHandle> model;

    GameObject()
    {
    }

    GameObject(const glm::mat4& transform, std::shared_ptr<ModelHandle> model)
        : transform(transform)
        , model(model)
    {
    }
};

struct SceneDescription
{
    Camera camera;
    std::vector<std::shared_ptr<ModelHandle>> models;
    std::vector<GameObject> gameObjects;
    DirectionalLight directionalLight;
};
