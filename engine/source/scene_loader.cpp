#include "scene_loader.hpp"

#include <entt/entity/entity.hpp>

#include "ECS.hpp"
#include "mesh.hpp"
#include "timers.hpp"
#include "vulkan_brain.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

void SceneLoader::LoadModelIntoECSAsHierarchy(const VulkanBrain& brain, ECS& ecs, const Model& model, std::vector<entt::entity>& entities)
{
    Stopwatch stopwatch;

    for (const auto& i : model.hierarchy.baseNodes)
    {
        entities.emplace_back(LoadNodeRecursive(brain, ecs, i));
    }

    bblog::info("loading model into scene took {} ms", stopwatch.GetElapsed().count());
}

entt::entity SceneLoader::LoadNodeRecursive(const VulkanBrain& brain, ECS& ecs, const Hierarchy::Node& currentNode)
{
    const entt::entity entity = ecs._registry.create();

    ecs._registry.emplace<NameComponent>(entity).name = currentNode.name;
    ecs._registry.emplace<TransformComponent>(entity);

    TransformHelpers::SetLocalTransform(ecs._registry, entity, currentNode.transform);
    ecs._registry.emplace<RelationshipComponent>(entity);

    if (brain.GetMeshResourceManager().IsValid(currentNode.mesh))
    {
        ecs._registry.emplace<StaticMeshComponent>(entity).mesh = currentNode.mesh;
    }

    for (const auto& node : currentNode.children)
    {
        const entt::entity childEntity = LoadNodeRecursive(brain, ecs, node);
        RelationshipHelpers::AttachChild(ecs._registry, entity, childEntity);
    }

    return entity;
}
