#pragma once

#include "animation.hpp"
#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>
#include <optional>

class GraphicsContext;
class ECSModule;

struct Animation;

namespace SceneLoading
{
entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const Hierarchy& hierarchy, std::optional<Animation> animation = std::nullopt);
};
