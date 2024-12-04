#pragma once

#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class GraphicsContext;
class ECSModule;

namespace SceneLoading
{
entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& modelResources, const Hierarchy& hierarchy);
};
