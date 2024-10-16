#pragma once

#include <entt/entity/entity.hpp>

#include "mesh.hpp"

struct ModelHandle;
class ECS;

class SceneLoader
{
public:
    static void LoadModelIntoECSAsHierarchy(const VulkanBrain& brain, ECS& ecs,const ModelHandle& model);
private:
    static entt::entity LoadNodeRecursive(const VulkanBrain& brain, ECS& ecs,const Hierarchy::Node& node);
};
