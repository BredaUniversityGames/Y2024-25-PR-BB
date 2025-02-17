#pragma once

#include "animation.hpp"
#include "cpu_resources.hpp"
#include "enum_utils.hpp"
#include <entt/entity/entity.hpp>

class GraphicsContext;
class ECSModule;

struct Animation;

namespace SceneLoading
{

enum class LoadFlags : uint8_t
{
    eLoadMeshes = 1 << 0,
    eLoadSkeletalMeshes = 1 << 1,
    eLoadCollision = 1 << 2,
    eLoadGameplayComponents = 1 << 3,
    eLoadLights = 1 << 4,
    eAll = eLoadMeshes | eLoadSkeletalMeshes | eLoadCollision | eLoadGameplayComponents | eLoadLights
};

GENERATE_ENUM_FLAG_OPERATORS(LoadFlags);

entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const CPUModel& cpuModel, const Hierarchy& hierarchy, std::vector<Animation> animations = {}, LoadFlags loadFlags = LoadFlags::eAll);
}
