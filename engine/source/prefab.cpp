//
// Created by luuk on 15-10-2024.
//

#include "prefab.hpp"

#include "ECS.hpp"
#include "mesh.hpp"

#include <entt/entity/entity.hpp>
void LoadModelIntoECSAsHierarchy(ECS& ecs,const ModelHandle& model)
{
    for (auto& node : model.hierarchy.allNodes)
    {
        entt::entity entity = ecs._registry.create();
        ecs._registry.emplace<NameComponent>(entity, node.name);
        ecs._registry.emplace<TransformComponent>(entity, node.transform);
        ecs._registry.emplace<StaticMeshComponent>(entity).mesh = node.mesh;
    }
}