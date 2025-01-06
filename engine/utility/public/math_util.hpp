#pragma once

#include <glm/gtc/quaternion.hpp>

void XYZWtoWXYZ(glm::quat& quat)
{
    std::swap(quat.w, quat.z);
    std::swap(quat.x, quat.z);
    std::swap(quat.y, quat.z);
}
