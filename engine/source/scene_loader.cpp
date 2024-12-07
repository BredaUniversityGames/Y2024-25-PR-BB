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

entt::entity LoadNodeRecursive(ECS& ecs, uint32_t currentNodeIndex, const Hierarchy& hierarchy, entt::entity parent, const GPUModel& model, std::shared_ptr<Animation> animation, std::unordered_map<uint32_t, entt::entity>& entityLUT, entt::entity skeletonRoot = entt::null)
{
    const entt::entity entity = ecs.registry.create();
    const Hierarchy::Node& currentNode = hierarchy.nodes[currentNodeIndex];

    entityLUT[currentNodeIndex] = entity;

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
        switch (currentNode.meshIndex.value().first)
        {
        case MeshType::eSTATIC:
            ecs.registry.emplace<StaticMeshComponent>(entity).mesh = model.meshes.at(currentNode.meshIndex.value().second);
            break;
        case MeshType::eSKINNED:
            ecs.registry.emplace<SkinnedMeshComponent>(entity).mesh = model.skinnedMeshes.at(currentNode.meshIndex.value().second);
            break;
        default:
            throw std::runtime_error("Mesh type not supported!");
        }
    }

    if (currentNode.animationChannel.has_value())
    {
        auto& animationChannel = ecs.registry.emplace<AnimationChannel>(entity) = currentNode.animationChannel.value();
        animationChannel.animation = animation;
    }

    if (currentNode.isSkeletonRoot)
    {
        ecs.registry.emplace<SkeletonComponent>(entity);
        skeletonRoot = entity;
    }

    if (currentNode.joint.has_value())
    {
        auto& joint = ecs.registry.emplace<JointComponent>(entity);
        joint.inverseBindMatrix = currentNode.joint.value().inverseBind;
        joint.jointIndex = currentNode.joint.value().index;
        assert(skeletonRoot != entt::null && "Joint requires a skeleton root, that should be present!");
        joint.skeletonEntity = skeletonRoot;
    }

    for (const auto& nodeIndex : currentNode.children)
    {
        LoadNodeRecursive(ecs, nodeIndex, hierarchy, entity, model, animation, entityLUT, skeletonRoot);
    }

    return entity;
}

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECS& ecs, const GPUModel& gpuModel, const Hierarchy& hierarchy, std::optional<Animation> animation)
{
    std::shared_ptr<Animation> animationControl { nullptr };
    if (animation.has_value())
    {
        animationControl = std::make_shared<Animation>(animation.value());
    }

    std::unordered_map<uint32_t, entt::entity> entityLUT;

    entt::entity rootEntity = LoadNodeRecursive(ecs, hierarchy.root, hierarchy, entt::null, gpuModel, animationControl, entityLUT);

    for (size_t i = 0; i < hierarchy.nodes.size(); ++i)
    {
        const Hierarchy::Node& node = hierarchy.nodes[i];
        if (node.skeletonNode.has_value() && node.meshIndex.has_value() && std::get<0>(node.meshIndex.value()) == MeshType::eSKINNED)
        {
            SkinnedMeshComponent& skinnedMeshComponent = ecs.registry.get<SkinnedMeshComponent>(entityLUT[i]);
            skinnedMeshComponent.skeletonEntity = entityLUT[node.skeletonNode.value()];
        }
    }

    return rootEntity;
}
