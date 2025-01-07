#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace math
{

constexpr glm::vec3 WORLD_UP { 0.0f, 1.0f, 0.0f };
constexpr glm::vec3 WORLD_RIGHT { 0.0f, 1.0f, 0.0f };
constexpr glm::vec3 WORLD_FORWARD { 0.0f, 0.0f, -1.0f };

inline void QuatXYZWtoWXYZ(glm::quat& quat)
{
    std::swap(quat.w, quat.z);
    std::swap(quat.x, quat.z);
    std::swap(quat.y, quat.z);
}

struct Vec3Range
{
    Vec3Range(const glm::vec3& newMin, const glm::vec3& newMax)
        : min(newMin)
        , max(newMax)
    {
    }

    // Initialized minimum to maximum possible value and maximum to minimum possible value
    Vec3Range()
    {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::lowest());
    }

    glm::vec3 min {};
    glm::vec3 max {};
};

struct URange
{
    uint32_t start;
    uint32_t count;
};

}