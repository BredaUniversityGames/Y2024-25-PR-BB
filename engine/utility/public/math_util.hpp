#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace math
{

constexpr glm::vec3 WORLD_UP { 0.0f, 1.0f, 0.0f };
constexpr glm::vec3 WORLD_RIGHT { 1.0f, 0.0f, 0.0f };
constexpr glm::vec3 WORLD_FORWARD { 0.0f, 0.0f, -1.0f };

inline void QuatXYZWtoWXYZ(glm::quat& quat)
{
    std::swap(quat.w, quat.z);
    std::swap(quat.x, quat.z);
    std::swap(quat.y, quat.z);
}

inline void QuatWXYZtoXYZW(glm::quat& quat)
{
    std::swap(quat.x, quat.z);
    std::swap(quat.y, quat.z);
    std::swap(quat.w, quat.z);
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

inline uint32_t DivideRoundingUp(uint32_t dividend, uint32_t divisor)
{
    return (dividend + divisor - 1) / divisor;
}

inline uint32_t RoundUpToPowerOfTwo(uint32_t n)
{
    if (n == 0)
    {
        return 1; // Special case: smallest power of two is 1
    }

    // If n is already a power of two, return it
    if ((n & (n - 1)) == 0)
    {
        return n;
    }

    // Round up to the next power of two
    n--; // Subtract 1 to handle exact powers of two correctly
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n + 1;
}

}