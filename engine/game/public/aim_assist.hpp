#pragma once

#include <glm/glm.hpp>

class ECSModule;
class PhysicsModule;

class AimAssist
{
public:
    static glm::vec3 GetAimAssistDirection(ECSModule& ecs, PhysicsModule& physics, const glm::vec3& forward);
};
