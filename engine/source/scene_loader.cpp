#include "scene_loader.hpp"

#include "animation.hpp"
#include "components/joint_component.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_mesh_component.hpp"
#include "components/static_mesh_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "vertex.hpp"

#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <single_time_commands.hpp>

entt::entity LoadNodeRecursive(ECSModule& ecs,
    uint32_t currentNodeIndex,
    const Hierarchy& hierarchy,
    entt::entity parent,
    const GPUModel& model,
    const CPUModel& cpuModel,
    std::shared_ptr<Animation> animation,
    std::unordered_map<uint32_t, entt::entity>& entityLUT, // Used for looking up from hierarchy node index to entt entity.
    entt::entity skeletonRoot = entt::null)
{
    const entt::entity entity = ecs.GetRegistry().create();
    const Hierarchy::Node& currentNode = hierarchy.nodes[currentNodeIndex];

    entityLUT[currentNodeIndex] = entity;

    ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name;
    ecs.GetRegistry().emplace<TransformComponent>(entity);

    ecs.GetRegistry().emplace<RelationshipComponent>(entity);
    if (parent != entt::null)
    {
        RelationshipHelpers::AttachChild(ecs.GetRegistry(), parent, entity);
    }

    TransformHelpers::SetLocalTransform(ecs.GetRegistry(), entity, currentNode.transform);

    if (currentNode.meshIndex.has_value())
    {
        switch (currentNode.meshIndex.value().first)
        {
        case MeshType::eSTATIC:
            ecs.GetRegistry().emplace<StaticMeshComponent>(entity).mesh = model.staticMeshes.at(currentNode.meshIndex.value().second);

            // check if it should have collider

            ecs.GetRegistry().emplace<RigidbodyComponent>(entity, ecs.GetSystem<PhysicsSystem>()->CreateMeshColliderBody(cpuModel.meshes.at(currentNode.meshIndex.value().second), PhysicsShapes::eCONVEXHULL, entity));

            // add collider recursively

            break;
        case MeshType::eSKINNED:
            ecs.GetRegistry().emplace<SkinnedMeshComponent>(entity).mesh = model.skinnedMeshes.at(currentNode.meshIndex.value().second);
            break;
        default:
            throw std::runtime_error("Mesh type not supported!");
        }
    }

    if (currentNode.animationChannel.has_value())
    {
        auto& animationChannel = ecs.GetRegistry().emplace<AnimationChannelComponent>(entity) = currentNode.animationChannel.value();
        animationChannel.animation = animation;
    }

    if (currentNode.isSkeletonRoot)
    {
        ecs.GetRegistry().emplace<SkeletonComponent>(entity);
        skeletonRoot = entity;
    }

    if (currentNode.joint.has_value())
    {
        auto& joint = ecs.GetRegistry().emplace<JointComponent>(entity);
        joint.inverseBindMatrix = currentNode.joint.value().inverseBind;
        joint.jointIndex = currentNode.joint.value().index;
        assert(skeletonRoot != entt::null && "Joint requires a skeleton root, that should be present!");
        joint.skeletonEntity = skeletonRoot;
    }

    for (const auto& nodeIndex : currentNode.children)
    {
        LoadNodeRecursive(ecs, nodeIndex, hierarchy, entity, model, cpuModel, animation, entityLUT, skeletonRoot);
    }

    return entity;
}

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const CPUModel& cpuModel, const Hierarchy& hierarchy, std::optional<Animation> animation)
{
    std::shared_ptr<Animation> animationControl { nullptr };
    if (animation.has_value())
    {
        animationControl = std::make_shared<Animation>(animation.value());
    }

    std::unordered_map<uint32_t, entt::entity> entityLUT;

    entt::entity rootEntity = LoadNodeRecursive(ecs, hierarchy.root, hierarchy, entt::null, gpuModel, cpuModel, animationControl, entityLUT);

    for (size_t i = 0; i < hierarchy.nodes.size(); ++i)
    {
        const Hierarchy::Node& node = hierarchy.nodes[i];
        if (node.skeletonNode.has_value() && node.meshIndex.has_value() && std::get<0>(node.meshIndex.value()) == MeshType::eSKINNED)
        {
            SkinnedMeshComponent& skinnedMeshComponent = ecs.GetRegistry().get<SkinnedMeshComponent>(entityLUT[i]);
            skinnedMeshComponent.skeletonEntity = entityLUT[node.skeletonNode.value()];
        }
    }

    return rootEntity;
}
