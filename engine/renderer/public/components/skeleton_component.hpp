#pragma once

#include <cstdint>

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

void AttachChild()
{
}
