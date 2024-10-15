//
// Created by luuk on 15-10-2024.
//

#include "prefab.hpp"

#include "ECS.hpp"
#include "mesh.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"

#include <entt/entity/entity.hpp>
void LoadModelIntoECSAsHierarchy(ECS& ecs, const ModelHandle& model)
{
    for (const auto& i : model.hierarchy.allNodes)
    {
        LoadNodeRecursive(ecs,i);
    }
}

entt::entity LoadNodeRecursive(ECS& ecs,const Hierarchy::Node& currentNode)
{
    entt::entity entity = ecs._registry.create();
    ecs._registry.emplace<NameComponent>(entity)._name = currentNode.name;
    ecs._registry.emplace<TransformComponent>(entity);
    
    TransformHelpers::SetLocalTransform(ecs._registry,entity,currentNode.transform);
    
    if(currentNode.mesh.index != 0xFFFFFF)
    {
        ecs._registry.emplace<StaticMeshComponent>(entity).mesh = currentNode.mesh;
    }

    ecs._registry.emplace<RelationshipComponent>(entity);
    
    for(const auto& i : currentNode.children)
    {   
        entt::entity childEntity = LoadNodeRecursive(ecs, i);
        RelationshipHelpers::AttachChild(ecs._registry, entity, childEntity);
    }
    return entity;
}
