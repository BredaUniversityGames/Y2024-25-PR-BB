#pragma once

#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class ECS;
class GraphicsContext;

namespace SceneLoading
{
std::vector<entt::entity> LoadModelIntoECSAsHierarchy(ECS& ecs, const GPUResources::Model& model);
};
