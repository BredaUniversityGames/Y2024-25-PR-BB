#include "scene_loader.hpp"

#include <entt/entity/entity.hpp>

#include "ecs.hpp"
#include "mesh.hpp"
#include "timers.hpp"
#include "vulkan_brain.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

std::vector<entt::entity> SceneLoader::LoadModelIntoECSAsHierarchy(const VulkanBrain& brain, ECS& ecs, const Model& model)
{
    Stopwatch stopwatch;

    std::vector<entt::entity> entities {};
    entities.reserve(model.hierarchy.baseNodes.size());

    for (const auto& i : model.hierarchy.baseNodes)
    {
        entities.emplace_back(LoadNodeRecursive(brain, ecs, i));
    }

    return entities;
}

entt::entity SceneLoader::LoadNodeRecursive(const VulkanBrain& brain, ECS& ecs, const Hierarchy::Node& currentNode)
{
    const entt::entity entity = ecs.registry.create();

    ecs.registry.emplace<NameComponent>(entity).name = currentNode.name;
    ecs.registry.emplace<TransformComponent>(entity);

    TransformHelpers::SetLocalTransform(ecs.registry, entity, currentNode.transform);
    ecs.registry.emplace<RelationshipComponent>(entity);

    if (brain.GetMeshResourceManager().IsValid(currentNode.mesh))
    {
        ecs.registry.emplace<StaticMeshComponent>(entity).mesh = currentNode.mesh;
    }

    for (const auto& node : currentNode.children)
    {
        const entt::entity childEntity = LoadNodeRecursive(brain, ecs, node);
        RelationshipHelpers::AttachChild(ecs.registry, entity, childEntity);
    }

    return entity;
}
