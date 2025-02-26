#include "physics/debug_renderer.hpp"

void PhysicsDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor)
{
    if (!_isEnabled)
    {
        return;
    }
    glm::vec3 fromPos(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
    glm::vec3 toPos(inTo.GetX(), inTo.GetY(), inTo.GetZ());

    linePositions.push_back(fromPos);
    linePositions.push_back(toPos);
}

void PhysicsDebugRenderer::AddPersistentLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor)
{
    if (!_isEnabled)
    {
        return;
    }

    glm::vec3 fromPos(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ());
    glm::vec3 toPos(inTo.GetX(), inTo.GetY(), inTo.GetZ());

    persistentLinePositions.push_back(fromPos);
    persistentLinePositions.push_back(toPos);
}