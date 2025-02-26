#pragma once
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <Jolt/Jolt.h>

inline glm::mat4 ToGLMMat4(const JPH::RMat44& mat)
{
    glm::mat4 glmMat;

    // JPH::RMat44 stores rotation columns and translation separately
    // mRotation is a 3x3 matrix, and mTranslation is a Vec3
    // GLM uses column-major order, so we can map the columns directly

    // Extract rotation columns from JPH::RMat44
    JPH::Vec3 col0 = mat.GetColumn3(0);
    JPH::Vec3 col1 = mat.GetColumn3(1);
    JPH::Vec3 col2 = mat.GetColumn3(2);
    JPH::Vec3 translation = mat.GetTranslation();

    // Set the columns of glm::mat4
    glmMat[0] = glm::vec4(col0.GetX(), col0.GetY(), col0.GetZ(), 0.0f);
    glmMat[1] = glm::vec4(col1.GetX(), col1.GetY(), col1.GetZ(), 0.0f);
    glmMat[2] = glm::vec4(col2.GetX(), col2.GetY(), col2.GetZ(), 0.0f);
    glmMat[3] = glm::vec4(translation.GetX(), translation.GetY(), translation.GetZ(), 1.0f);

    return glmMat;
}

inline glm::vec3 ToGLMVec3(const JPH::Vec3& vec)
{
    return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
}

inline JPH::Vec3 ToJoltVec3(const glm::vec3& vec)
{
    return { vec.x, vec.y, vec.z };
}

inline JPH::Quat ToJoltQuat(const glm::quat& q)
{
    return { q.x, q.y, q.z, q.w };
}

inline glm::quat ToGLMQuat(const JPH::Quat& quat)
{
    return glm::quat { quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ() };
}