#pragma once
#include <glm/vec3.hpp>

struct Camera
{
    enum class Projection
    {
        ePerspective,
        eOrthographic
    } projection;

    glm::vec3 position {};
    glm::vec3 eulerRotation {};
    float fov {};

    float orthographicSize;

    float nearPlane {};
    float farPlane {};
    float aspectRatio {};
};
