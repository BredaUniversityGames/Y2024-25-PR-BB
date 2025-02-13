#include "scene/scene_loader.hpp"

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

#include "systems/physics_system.hpp"
#include "vertex.hpp"

#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <single_time_commands.hpp>

void ProcessNodeGameplayComponents(const Hierarchy::Node& node, MAYBE_UNUSED ECSModule& ecs, MAYBE_UNUSED entt::entity currentEntity)
{
    // enemy spawners
    if (node.name.starts_with("ESPWN_"))
    {
        assert(false && "Enemy spawner component not yet implemented");
        // ecs.GetRegistry().emplace<EnemySpawnerComponent>(currentEntity);
    }

    // player spawn point
    // note: if there are multiple spawnpoints in the given hierarchy then the last one to be processed will be the actual player spawn point.
    if (node.name.starts_with("PSPWN_"))
    {
        assert(false && "Setting player spawn not yet implemented");
    }
}

void LoadNodeRecursive(ECSModule& ecs,
    entt::entity entity,
    uint32_t currentNodeIndex,
    const Hierarchy& hierarchy,
    entt::entity parent,
    const GPUModel& model,
    const CPUModel& cpuModel,
    AnimationControlComponent* animationControl,
    std::unordered_map<uint32_t, entt::entity>& entityLUT, // Used for looking up from hierarchy node index to entt entity.
    entt::entity skeletonRoot = entt::null,
    bool isSkeletonRoot = false, SceneLoading::LoadFlags loadFlags = SceneLoading::LoadFlags::eAll)
{
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

    // Static/skeletal meshes
    if (currentNode.meshIndex.has_value())
    {
        switch (currentNode.meshIndex.value().first)
        {
        case MeshType::eSTATIC:
            ecs.GetRegistry().emplace<StaticMeshComponent>(entity).mesh = model.staticMeshes.at(currentNode.meshIndex.value().second);

            // check if it should have collider
            if (HasAnyFlags(loadFlags, SceneLoading::LoadFlags::eLoadCollision))
            {
                ecs.GetRegistry().emplace<RigidbodyComponent>(entity, ecs.GetSystem<PhysicsSystem>()->CreateMeshColliderBody(cpuModel.meshes.at(currentNode.meshIndex.value().second), PhysicsShapes::eCONVEXHULL, entity));
            }
            break;

        case MeshType::eSKINNED:

            if (HasAnyFlags(loadFlags, SceneLoading::LoadFlags::eLoadSkeletalMeshes))
            {
                ecs.GetRegistry().emplace<SkinnedMeshComponent>(entity).mesh = model.skinnedMeshes.at(currentNode.meshIndex.value().second);
            }
            break;

        default:
            throw std::runtime_error("Mesh type not supported!");
        }
    }

    if (HasAnyFlags(loadFlags, SceneLoading::LoadFlags::eLoadSkeletalMeshes))
    {
        if (!currentNode.animationSplines.empty())
        {
            assert(animationControl != nullptr);

            auto& animationChannel = ecs.GetRegistry().emplace<AnimationChannelComponent>(entity);
            animationChannel.animationSplines = currentNode.animationSplines;
            animationChannel.animationControl = animationControl;
        }

        if (isSkeletonRoot)
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
    }

    // Gameplay components.
    if (HasAnyFlags(loadFlags, SceneLoading::LoadFlags::eLoadGameplayComponents))
    {
        ProcessNodeGameplayComponents(hierarchy.nodes[currentNodeIndex], ecs, entity);
    }

    for (const auto& nodeIndex : currentNode.children)
    {
        const entt::entity childEntity = ecs.GetRegistry().create();
        LoadNodeRecursive(ecs, childEntity, nodeIndex, hierarchy, entity, model, cpuModel, animationControl, entityLUT, skeletonRoot, false, loadFlags);
    }
}

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const CPUModel& cpuModel, const Hierarchy& hierarchy, std::vector<Animation> animations, LoadFlags loadFlags)
{
    entt::entity rootEntity = ecs.GetRegistry().create();

    std::unordered_map<uint32_t, entt::entity> entityLUT;

    AnimationControlComponent* animationControl = nullptr;
    if (!animations.empty() && HasAnyFlags(loadFlags, SceneLoading::LoadFlags::eLoadSkeletalMeshes))
    {
        animationControl = &ecs.GetRegistry().emplace<AnimationControlComponent>(rootEntity, animations, std::nullopt);
    }

    LoadNodeRecursive(ecs, rootEntity, hierarchy.root, hierarchy, entt::null, gpuModel, cpuModel, animationControl, entityLUT, entt::null, false, loadFlags);

    if (HasAnyFlags(loadFlags, SceneLoading::LoadFlags::eLoadSkeletalMeshes))
    {

        if (hierarchy.skeletonRoot.has_value())
        {
            entt::entity skeletonEntity = ecs.GetRegistry().create();

            // Note: load twice?
            LoadNodeRecursive(ecs, skeletonEntity, hierarchy.skeletonRoot.value(), hierarchy, entt::null, gpuModel, cpuModel, animationControl, entityLUT, entt::null, true, loadFlags);
            RelationshipHelpers::AttachChild(ecs.GetRegistry(), rootEntity, skeletonEntity);
        }

        for (size_t i = 0; i < hierarchy.nodes.size(); ++i)
        {
            const Hierarchy::Node& node = hierarchy.nodes[i];
            if (node.skeletonNode.has_value() && node.meshIndex.has_value() && std::get<0>(node.meshIndex.value()) == MeshType::eSKINNED)
            {
                SkinnedMeshComponent& skinnedMeshComponent = ecs.GetRegistry().get<SkinnedMeshComponent>(entityLUT[i]);
                skinnedMeshComponent.skeletonEntity = entityLUT[node.skeletonNode.value()];
            }
        }
    }
    return rootEntity;
}
