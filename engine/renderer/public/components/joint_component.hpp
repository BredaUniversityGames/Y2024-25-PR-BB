#pragma once

#include <cstdint>
#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>

struct JointComponent
{
    glm::mat4 inverseBindMatrix {};
    uint32_t jointIndex {};
    entt::entity skeletonEntity { entt::null };
};
