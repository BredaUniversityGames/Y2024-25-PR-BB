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

#include "components/animation_transform_component.hpp"
#include "systems/physics_system.hpp"
#include "vertex.hpp"

#include <entt/entity/entity.hpp>
#include <glm/glm.hpp>
#include <single_time_commands.hpp>

class RecursiveNodeLoader
{
public:
    RecursiveNodeLoader(ECSModule& ecs, const Hierarchy& hierarchy, const CPUModel& cpuModel, const GPUModel& gpuModel, AnimationControlComponent* animationControl, std::unordered_map<uint32_t, entt::entity>& entityLut)
        : _ecs(ecs)
        , _hierarchy(hierarchy)
        , _cpuModel(cpuModel)
        , _gpuModel(gpuModel)
        , _animationControl(animationControl)
        , _entityLUT(entityLut)
    {
    }

    void Load(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        const Hierarchy::Node& currentNode = _hierarchy.nodes[currentNodeIndex];

        _entityLUT[currentNodeIndex] = entity;

        _ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name;
        _ecs.GetRegistry().emplace<TransformComponent>(entity);

        _ecs.GetRegistry().emplace<RelationshipComponent>(entity);
        if (parent != entt::null)
        {
            RelationshipHelpers::AttachChild(_ecs.GetRegistry(), parent, entity);
        }

        TransformHelpers::SetLocalTransform(_ecs.GetRegistry(), entity, currentNode.transform);

        if (currentNode.meshIndex.has_value())
        {
            switch (currentNode.meshIndex.value().first)
            {
            case MeshType::eSTATIC:
                _ecs.GetRegistry().emplace<StaticMeshComponent>(entity).mesh = _gpuModel.staticMeshes.at(currentNode.meshIndex.value().second);

                // check if it should have collider

                _ecs.GetRegistry().emplace<RigidbodyComponent>(entity, _ecs.GetSystem<PhysicsSystem>()->CreateMeshColliderBody(_cpuModel.meshes.at(currentNode.meshIndex.value().second), PhysicsShapes::eCONVEXHULL, entity));

                // add collider recursively

                break;
            case MeshType::eSKINNED:
                _ecs.GetRegistry().emplace<SkinnedMeshComponent>(entity).mesh = _gpuModel.skinnedMeshes.at(currentNode.meshIndex.value().second);
                break;
            default:
                throw std::runtime_error("Mesh type not supported!");
            }
        }

        if (!currentNode.animationSplines.empty())
        {
            assert(_animationControl != nullptr);

            auto& animationChannel = _ecs.GetRegistry().emplace<AnimationChannelComponent>(entity);
            animationChannel.animationSplines = currentNode.animationSplines;
            animationChannel.animationControl = _animationControl;
        }

        if (currentNode.joint.has_value())
        {
            assert(!"Joints should only appear in the skeleton");
        }

        for (const auto& nodeIndex : currentNode.children)
        {
            const entt::entity childEntity = _ecs.GetRegistry().create();
            Load(childEntity, nodeIndex, entity);
        }
    }

private:
    ECSModule& _ecs;
    const Hierarchy& _hierarchy;
    const CPUModel& _cpuModel;
    const GPUModel& _gpuModel;
    AnimationControlComponent* _animationControl;
    std::unordered_map<uint32_t, entt::entity>& _entityLUT;

    SkeletonComponent* _skeletonComponent;
};

class RecursiveSkeletonLoader
{
public:
    RecursiveSkeletonLoader(ECSModule& ecs, const Hierarchy& hierarchy, AnimationControlComponent* animationControl, std::unordered_map<uint32_t, entt::entity>& entityLUT)
        : _ecs(ecs)
        , _hierarchy(hierarchy)
        , _animationControl(animationControl)
        , _entityLUT(entityLUT)
    {
    }

    void Load(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        _skeletonComponent = &_ecs.GetRegistry().emplace<SkeletonComponent>(entity);
        _skeletonEntity = entity;

        LoadNode(entity, currentNodeIndex, parent);
    }

private:
    ECSModule& _ecs;
    const Hierarchy& _hierarchy;
    AnimationControlComponent* _animationControl;
    std::unordered_map<uint32_t, entt::entity>& _entityLUT;

    SkeletonComponent* _skeletonComponent;
    entt::entity _skeletonEntity;

    void LoadNode(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        const Hierarchy::Node& currentNode = _hierarchy.nodes[currentNodeIndex];

        _ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name;
        _ecs.GetRegistry().emplace<AnimationTransformComponent>(entity);
        auto& skeletonNode = _ecs.GetRegistry().emplace<SkeletonNodeComponent>(entity);
        SkeletonHelpers::InitializeSkeletonNode(skeletonNode);

        if (parent != entt::null)
        {
            SkeletonHelpers::AttachChild(_ecs.GetRegistry(), parent, entity);
        }

        AnimationTransformHelpers::SetLocalTransform(_ecs.GetRegistry(), entity, currentNode.transform);

        if (!currentNode.animationSplines.empty())
        {
            assert(_animationControl != nullptr);

            auto& animationChannel = _ecs.GetRegistry().emplace<AnimationChannelComponent>(entity);
            animationChannel.animationSplines = currentNode.animationSplines;
            animationChannel.animationControl = _animationControl;
        }

        if (currentNode.joint.has_value())
        {
            auto& joint = _ecs.GetRegistry().emplace<JointComponent>(entity);
            joint.inverseBindMatrix = currentNode.joint.value().inverseBind;
            joint.jointIndex = currentNode.joint.value().index;
            joint.skeletonEntity = _skeletonEntity;
        }

        for (const auto& nodeIndex : currentNode.children)
        {
            const entt::entity childEntity = _ecs.GetRegistry().create();
            LoadNode(childEntity, nodeIndex, entity);
        }
    }
};

entt::entity SceneLoading::LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const CPUModel& cpuModel, const Hierarchy& hierarchy, std::vector<Animation> animations)
{
    entt::entity rootEntity = ecs.GetRegistry().create();

    std::unordered_map<uint32_t, entt::entity> entityLUT;

    AnimationControlComponent* animationControl = nullptr;
    if (!animations.empty())
    {
        animationControl = &ecs.GetRegistry().emplace<AnimationControlComponent>(rootEntity, animations, std::nullopt);
    }

    RecursiveNodeLoader recursiveNodeLoader { ecs, hierarchy, cpuModel, gpuModel, animationControl, entityLUT };
    recursiveNodeLoader.Load(rootEntity, hierarchy.root, entt::null);

    if (hierarchy.skeletonRoot.has_value())
    {
        entt::entity skeletonEntity = ecs.GetRegistry().create();
        ecs.GetRegistry().emplace<TransformComponent>(skeletonEntity);
        ecs.GetRegistry().emplace<RelationshipComponent>(skeletonEntity);

        RecursiveSkeletonLoader recursiveSkeletonLoader { ecs, hierarchy, animationControl, entityLUT };
        recursiveSkeletonLoader.Load(skeletonEntity, hierarchy.skeletonRoot.value(), entt::null);
        RelationshipHelpers::AttachChild(ecs.GetRegistry(), rootEntity, skeletonEntity);
    }

    // TODO: Broken
    // Links skeleton entities to the skinned mesh components.
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
