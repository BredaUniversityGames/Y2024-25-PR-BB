#pragma once

#include "animation.hpp"
#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>
#include <optional>

class ECS;
class GraphicsContext;
struct Animation;

namespace SceneLoading
{
entt::entity LoadModelIntoECSAsHierarchy(ECS& ecs, const GPUModel& modelResources, const Hierarchy& hierarchy, std::optional<Animation> animation = std::nullopt);
};
