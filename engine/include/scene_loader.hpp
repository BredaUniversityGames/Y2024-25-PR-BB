#pragma once

#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class GraphicsContext;
class ECSModule;

namespace SceneLoading
{
entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, const CPUModel& cpuModel, const GPUModel& modelResources, const Hierarchy& hierarchy);
};
