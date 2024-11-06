#pragma once

#include <entt/entity/entity.hpp>

#include "model.hpp"
#include "vulkan_brain.hpp"

class ECS;

class SceneLoader
{
public:
    static void LoadModelIntoECSAsHierarchy(const VulkanBrain& brain, ECS& ecs, const Model& model, std::vector<entt::entity>& entities);

private:
    static entt::entity LoadNodeRecursive(const VulkanBrain& brain, ECS& ecs, const Hierarchy::Node& node);
};
