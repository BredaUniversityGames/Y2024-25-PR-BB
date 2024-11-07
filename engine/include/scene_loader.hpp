#pragma once

#include <entt/entity/entity.hpp>

#include "model.hpp"
#include "vulkan_brain.hpp"

class ECS;

class SceneLoader
{
public:
    std::vector<entt::entity> LoadModelIntoECSAsHierarchy(const VulkanBrain& brain, ECS& ecs, const Model& model);

private:
    entt::entity LoadNodeRecursive(const VulkanBrain& brain, ECS& ecs, const Hierarchy::Node& node);
};
