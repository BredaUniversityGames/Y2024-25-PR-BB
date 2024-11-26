#include "scene_loader.hpp"

#include "animation.hpp"
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

entt::entity LoadNodeRecursive(ECS& ecs, const Hierarchy::Node& currentNode, const GPUModel& model, std::shared_ptr<Animation> animation)
{
    const entt::entity entity = ecs.registry.create();

    ecs.registry.emplace<NameComponent>(entity).name = currentNode.name;
    ecs.registry.emplace<TransformComponent>(entity);

    TransformHelpers::SetLocalTransform(ecs.registry, entity, currentNode.transform);
    ecs.registry.emplace<RelationshipComponent>(entity);

    if (currentNode.meshIndex.has_value())
    {
        ecs.registry.emplace<StaticMeshComponent>(entity).mesh = model.meshes.at(currentNode.meshIndex.value());
    }

    if (currentNode.animationChannel.has_value())
    {
        auto& animationChannel = ecs.registry.emplace<AnimationChannel>(entity) = currentNode.animationChannel.value();
        animationChannel.animation = animation;
    }

    if (currentNode.joint.has_value())
    {
        ecs.registry.emplace<JointComponent>(entity).inverseBindMatrix = currentNode.joint.value().inverseBind;

        if (currentNode.joint.value().isSkeletonRoot)
        {
            ecs.registry.emplace<SkeletonComponent>(entity);
        }
    }

    for (const auto& node : currentNode.children)
    {
        const entt::entity childEntity = LoadNodeRecursive(ecs, node, model, animation);
        RelationshipHelpers::AttachChild(ecs.registry, entity, childEntity);
    }

    return entity;
}

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECS& ecs, const GPUModel& modelResources, const Hierarchy& hierarchy, std::optional<Animation> animation)
{
    auto baseNode = hierarchy.baseNodes.at(0);

    std::shared_ptr<Animation> animationControl { nullptr };
    if (animation.has_value())
    {
        animationControl = std::make_shared<Animation>(animation.value());
    }

    return LoadNodeRecursive(ecs, baseNode, modelResources, animationControl);
}
