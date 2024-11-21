#include "scene_loader.hpp"

#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "mesh.hpp"
#include "resource_management/mesh_resource_manager.hpp"

#include <entt/entity/entity.hpp>
#include <single_time_commands.hpp>

entt::entity LoadNodeRecursive(ECS& ecs, const Hierarchy::Node& currentNode, const GPUResources::Model& model)
{
    const entt::entity entity = ecs.registry.create();

    std::cout << "sdfadw" << std::endl;
    ecs.registry.emplace<NameComponent>(entity).name = currentNode.name;
    ecs.registry.emplace<TransformComponent>(entity);

    TransformHelpers::SetLocalTransform(ecs.registry, entity, currentNode.transform);
    ecs.registry.emplace<RelationshipComponent>(entity);

    if (currentNode.meshIndex.has_value())
    {
        ecs.registry.emplace<StaticMeshComponent>(entity).mesh = model.meshes.at(currentNode.meshIndex.value());
    }

    for (const auto& node : currentNode.children)
    {
        const entt::entity childEntity = LoadNodeRecursive(ecs, node, model);
        RelationshipHelpers::AttachChild(ecs.registry, entity, childEntity);
    }

    return entity;
}

std::vector<entt::entity> SceneLoading::LoadModelIntoECSAsHierarchy(ECS& ecs, const GPUResources::Model& model)
{
    std::vector<entt::entity> entities {};
    std::cout << "  dfrster 8" << std::endl;

    for (const auto& i : model.hierarchy.baseNodes)
    {
        entities.emplace_back(LoadNodeRecursive(ecs, i, model));
    }

    return entities;
}
