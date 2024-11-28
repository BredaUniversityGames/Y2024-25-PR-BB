#include "scene_loader.hpp"

#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "mesh.hpp"
#include "resource_management/mesh_resource_manager.hpp"

#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <single_time_commands.hpp>

entt::entity LoadNodeRecursive(ECS& ecs, const Hierarchy::Node& currentNode, entt::entity parent, const CPUModel& cpuModel, const GPUModel& model)
{
    const entt::entity entity = ecs.registry.create();

    ecs.registry.emplace<NameComponent>(entity).name = currentNode.name;
    ecs.registry.emplace<TransformComponent>(entity);

    ecs.registry.emplace<RelationshipComponent>(entity);

    if (parent != entt::null)
    {
        RelationshipHelpers::AttachChild(ecs.registry, parent, entity);
    }
    TransformHelpers::SetLocalTransform(ecs.registry, entity, currentNode.transform);

    if (currentNode.meshIndex.has_value())
    {
        ecs.registry.emplace<StaticMeshComponent>(entity).mesh = model.meshes.at(currentNode.meshIndex.value());

        TempPhysicsData tempData;
        tempData.boundingBox = cpuModel.meshes.at(currentNode.meshIndex.value()).GetMeshBounds();

        ecs.registry.emplace<TempPhysicsData>(entity, tempData);
    }

    for (const auto& node : currentNode.children)
    {
        const entt::entity childEntity = LoadNodeRecursive(ecs, node, entity, cpuModel, model);
        RelationshipHelpers::AttachChild(ecs.registry, entity, childEntity);
    }

    return entity;
}

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECS& ecs, const CPUModel& cpuModel, const GPUModel& modelResources, const Hierarchy& hierarchy)
{
    auto baseNode = hierarchy.baseNodes.at(0);
    return (LoadNodeRecursive(ecs, baseNode, entt::null, cpuModel, modelResources));
}
