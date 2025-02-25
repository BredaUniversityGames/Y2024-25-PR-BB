#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

struct SkeletonNodeComponent
{
    entt::entity parent { entt::null };
    std::array<entt::entity, 7> children;
};

struct SkeletonComponent
{
    entt::entity root { entt::null };
};

struct JointWorldTransformComponent
{
    glm::mat4 world { glm::identity<glm::mat4>() };
};

struct JointSkinDataComponent
{
    glm::mat4 inverseBindMatrix {};
    uint32_t jointIndex {};
    entt::entity skeletonEntity { entt::null };
};

namespace SkeletonHelpers
{
void AttachChild(entt::registry& registry, entt::entity parent, entt::entity child);
void InitializeSkeletonNode(SkeletonNodeComponent& node);
}
