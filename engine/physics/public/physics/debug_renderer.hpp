#pragma once
#include "common.hpp"
#include <Jolt/Jolt.h>

#include <Jolt/Renderer/DebugRendererSimple.h>
#include <glm/vec3.hpp>
#include <vector>

class PhysicsDebugRenderer : public JPH::DebugRendererSimple
{
public:
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor) override;
    void AddPersistentLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, MAYBE_UNUSED JPH::ColorArg inColor);

    void DrawText3D(
        MAYBE_UNUSED JPH::RVec3Arg inPosition,
        MAYBE_UNUSED const std::string_view& inString,
        MAYBE_UNUSED JPH::ColorArg inColor,
        MAYBE_UNUSED float inHeight) override {};
    // {
    //     // Not implemented
    // }

    NO_DISCARD const std::vector<glm::vec3>& GetLinesData() const { return linePositions; }
    NO_DISCARD const std::vector<glm::vec3>& GetPersistentLinesData() const { return persistentLinePositions; }
    void ClearLines() { linePositions.clear(); }

    void SetState(const bool newState) { _isEnabled = newState; }
    bool GetState() const { return _isEnabled; }

private:
    std::vector<glm::vec3> linePositions;
    std::vector<glm::vec3> persistentLinePositions;
    bool _isEnabled = false;
};