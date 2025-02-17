#pragma once

#include <cstdint>
#include <entt/entt.hpp>

struct SkeletonNodeComponent
{
    entt::entity parent;
    std::array<entt::entity, 7> children; // TODO: Check optimal size.
};

struct SkeletonComponent
{
    entt::entity root;
    std::vector<entt::entity> nodes;
    uint32_t boneOffset = 0;
};

namespace SkeletonHelpers
{

void AttachChild(entt::registry& registry, entt::entity parent, entt::entity child);
void InitializeSkeletonNode(SkeletonNodeComponent& node);
}
