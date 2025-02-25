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
};

struct JointWorldTransformComponent
{
    glm::mat4 world;
};

struct JointSkinDataComponent
{
    glm::mat4 inverseBindMatrix {}; // TODO: Is constant over each mesh
    uint32_t jointIndex {};
    entt::entity skeletonEntity { entt::null };
};

namespace SkeletonHelpers
{
void AttachChild(entt::registry& registry, entt::entity parent, entt::entity child);
void InitializeSkeletonNode(SkeletonNodeComponent& node);
}
