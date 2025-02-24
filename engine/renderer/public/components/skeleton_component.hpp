#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>

struct SkeletonNodeComponent
{
    entt::entity parent;
    std::array<entt::entity, 7> children; // TODO: Check optimal size.
};

struct SkeletonComponent
{
    entt::entity root;
    uint32_t boneOffset = 0;
};

struct SkeletonMatrixTransform
{
    glm::mat4 world;
};

namespace SkeletonHelpers
{

void AttachChild(entt::registry& registry, entt::entity parent, entt::entity child);
void InitializeSkeletonNode(SkeletonNodeComponent& node);
}
