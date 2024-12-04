#pragma once
#include <glm/vec3.hpp>

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