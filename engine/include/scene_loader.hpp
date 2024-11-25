#pragma once

#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class ECS;
class GraphicsContext;

namespace SceneLoading
{
entt::entity LoadModelIntoECSAsHierarchy(ECS& ecs, const GPUModel& model);
};
