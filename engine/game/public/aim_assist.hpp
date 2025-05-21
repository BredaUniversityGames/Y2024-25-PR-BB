#pragma once

#include <glm/glm.hpp>

class ECSModule;

class AimAssist
{
public:
  static glm::vec3 GetAimAssistDirection(ECSModule& ecs, const glm::vec3& forward);
};
