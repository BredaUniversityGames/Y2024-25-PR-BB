#pragma once

#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class ECS;
class VulkanContext;

class SceneLoader
{
public:
    std::vector<entt::entity> LoadModelIntoECSAsHierarchy(const std::shared_ptr<VulkanContext>& context, ECS& ecs, const Model& model);

private:
    entt::entity LoadNodeRecursive(const std::shared_ptr<VulkanContext>& context, ECS& ecs, const Hierarchy::Node& node);
};
