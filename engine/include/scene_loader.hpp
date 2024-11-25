#pragma once

#include "model.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class ECS;
class GraphicsContext;
struct Animation;

class SceneLoader
{
public:
    std::vector<entt::entity> LoadModelIntoECSAsHierarchy(const std::shared_ptr<GraphicsContext>& context, ECS& ecs, const Model& model);

private:
    entt::entity LoadNodeRecursive(const std::shared_ptr<GraphicsContext>& context, ECS& ecs, const Hierarchy::Node& node, std::shared_ptr<Animation> animation);
};
