#pragma once

#include "gpu_resources.hpp"
#include "resources/model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>
#include <optional>

class GraphicsContext;
class ECSModule;

struct Animation;

namespace SceneLoading
{
entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const CPUModel& cpuModel, const Hierarchy& hierarchy, std::vector<Animation> animations = {});
};
