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

std::vector<entt::entity> SceneLoader::LoadModelIntoECSAsHierarchy(const std::shared_ptr<GraphicsContext>& context, ECS& ecs, const Model& model)
{
    std::vector<entt::entity> entities {};
    entities.reserve(model.hierarchy.baseNodes.size());

    std::shared_ptr<Animation> animation { nullptr };
    if (model.animation.has_value())
    {
        animation = std::make_shared<Animation>(model.animation.value());
    }

    for (const auto& i : model.hierarchy.baseNodes)
    {
        entities.emplace_back(LoadNodeRecursive(context, ecs, i, animation));
    }

    return entities;
}

entt::entity SceneLoader::LoadNodeRecursive(const std::shared_ptr<GraphicsContext>& context, ECS& ecs, const Hierarchy::Node& currentNode, std::shared_ptr<Animation> animation)
{
    const entt::entity entity = ecs.registry.create();

    ecs.registry.emplace<NameComponent>(entity).name = currentNode.name;
    ecs.registry.emplace<TransformComponent>(entity);

    TransformHelpers::SetLocalTransform(ecs.registry, entity, currentNode.transform);
    ecs.registry.emplace<RelationshipComponent>(entity);

    if (context->Resources()->MeshResourceManager().IsValid(currentNode.mesh))
    {
        ecs.registry.emplace<StaticMeshComponent>(entity).mesh = currentNode.mesh;
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
        const entt::entity childEntity = LoadNodeRecursive(context, ecs, node, animation);
        RelationshipHelpers::AttachChild(ecs.registry, entity, childEntity);
    }

    return entity;
}
