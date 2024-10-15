#pragma once
#include "mesh.hpp"

#include <entt/entity/entity.hpp>

struct ModelHandle;
class ECS;


void LoadModelIntoECSAsHierarchy(ECS& ecs,const ModelHandle& model);
entt::entity LoadNodeRecursive(ECS& ecs,const Hierarchy::Node& node);