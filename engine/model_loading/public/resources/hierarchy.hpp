#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <resources/mesh.hpp>

enum class NodeLightType
{
    Directional,
    Point,
    Spot
};

struct NodeLightData
{
    static constexpr float DEFAULT_LIGHT_RANGE = 16.f;

    glm::vec3 color {};
    NodeLightType type {};
    float range {};
    float intensity {};
};

struct NodeMeshData
{
    MeshType type {};
    uint32_t index {};
};

struct NodeJointData
{
    uint32_t index {};
    glm::mat4 inverseBind = glm::identity<glm::mat4>();
};

struct Node
{
    std::string name {};
    glm::mat4 transform { 1.0f };
    std::vector<uint32_t> childrenIndices {};

    std::optional<std::pair<MeshType, uint32_t>> meshIndex {};
    std::optional<NodeLightData> light {};
    std::optional<NodeJointData> joint {};

    std::unordered_map<uint32_t, TransformAnimationSpline> animationSplines {};
    std::optional<uint32_t> skeletonNode {};
};

struct Hierarchy
{
    uint32_t root {};
    std::vector<Node> nodes {};

    std::optional<uint32_t> skeletonRoot = std::nullopt;
};
