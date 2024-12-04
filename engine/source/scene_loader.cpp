#include "scene_loader.hpp"

#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "mesh.hpp"
#include "resource_management/mesh_resource_manager.hpp"

#include <entt/entity/entity.hpp>
#include <single_time_commands.hpp>

entt::entity LoadNodeRecursive(ECSModule& ecs, const Hierarchy::Node& currentNode, const GPUModel& model)
{
    const entt::entity entity = ecs.GetRegistry().create();

    ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name;
    ecs.GetRegistry().emplace<TransformComponent>(entity);

    TransformHelpers::SetLocalTransform(ecs.GetRegistry(), entity, currentNode.transform);
    ecs.GetRegistry().emplace<RelationshipComponent>(entity);

    if (currentNode.meshIndex.has_value())
    {
        ecs.GetRegistry().emplace<StaticMeshComponent>(entity).mesh = model.meshes.at(currentNode.meshIndex.value());
    }

    for (const auto& node : currentNode.children)
    {
        const entt::entity childEntity = LoadNodeRecursive(ecs, node, model);
        RelationshipHelpers::AttachChild(ecs.GetRegistry(), entity, childEntity);
    }

    return entity;
}

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& modelResources, const Hierarchy& hierarchy)
{
    auto baseNode = hierarchy.baseNodes.at(0);
    return (LoadNodeRecursive(ecs, baseNode, modelResources));
}
