#include "scene/scene_loader.hpp"

#include "animation.hpp"
#include "components/animation_transform_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/is_static_draw.hpp"
#include "components/name_component.hpp"
#include "components/point_light_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_mesh_component.hpp"
#include "components/static_mesh_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "cpu_resources.hpp"
#include "ecs_module.hpp"
#include "model_loading.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "resource_management/model_resource_manager.hpp"
#include "systems/physics_system.hpp"
#include "thread_module.hpp"

#include <entt/entity/entity.hpp>
#include <tracy/Tracy.hpp>

class RecursiveNodeLoader
{
public:
    RecursiveNodeLoader(ECSModule& ecs, const Hierarchy& hierarchy, const CPUModel& cpuModel, const GPUModel& gpuModel, entt::entity animationControlEntity, std::unordered_map<uint32_t, entt::entity>& entityLut)
        : _ecs(ecs)
        , _hierarchy(hierarchy)
        , _cpuModel(cpuModel)
        , _gpuModel(gpuModel)
        , _animationControlEntity(animationControlEntity)
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
            {
                _ecs.GetRegistry().emplace<StaticMeshComponent>(entity).mesh = _gpuModel.staticMeshes.at(currentNode.meshIndex.value().second);
                _ecs.GetRegistry().emplace<IsStaticDraw>(entity);

                // check if it should have collider

                auto rb = _ecs.GetSystem<PhysicsSystem>()->CreateMeshColliderBody(_cpuModel.meshes.at(currentNode.meshIndex.value().second), PhysicsShapes::eMESH);
                _ecs.GetRegistry().emplace<RigidbodyComponent>(entity, rb);

                // add collider recursively

                break;
            }
            case MeshType::eSKINNED:
                _ecs.GetRegistry().emplace<SkinnedMeshComponent>(entity).mesh = _gpuModel.skinnedMeshes.at(currentNode.meshIndex.value().second);
                break;
            default:
                throw std::runtime_error("Mesh type not supported!");
            }
        }

        if (!currentNode.animationSplines.empty())
        {
            assert(_animationControlEntity != entt::null);

            auto& animationChannel = _ecs.GetRegistry().emplace<AnimationChannelComponent>(entity);
            animationChannel.animationSplines = currentNode.animationSplines;
            animationChannel.animationControlEntity = _animationControlEntity;
        }

        if (currentNode.joint.has_value())
        {
            assert(!"Joints should only appear in the skeleton");
        }

        if (currentNode.light.has_value())
        {
            switch (currentNode.light->type)
            {
            case Hierarchy::LightData::LightType::Directional:
            {
                auto& directionalLight = _ecs.GetRegistry().emplace<DirectionalLightComponent>(entity);
                directionalLight.color = currentNode.light->color * currentNode.light->intensity;
                break;
            }
            case Hierarchy::LightData::LightType::Point:
            {
                auto& pointLight = _ecs.GetRegistry().emplace<PointLightComponent>(entity);
                pointLight.color = currentNode.light->color;
                pointLight.intensity = currentNode.light->intensity;
                pointLight.range = currentNode.light->range;
                break;
            }
            case Hierarchy::LightData::LightType::Spot:
            {
                assert(!"Spot lights are not supported!");
                break;
            }
            }
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
    entt::entity _animationControlEntity;
    std::unordered_map<uint32_t, entt::entity>& _entityLUT;
};

class RecursiveSkeletonLoader
{
public:
    RecursiveSkeletonLoader(ECSModule& ecs, const Hierarchy& hierarchy, entt::entity animationControlEntity)
        : _ecs(ecs)
        , _hierarchy(hierarchy)
        , _animationControlEntity(animationControlEntity)
        , _skeletonComponent(nullptr)
    {
    }

    void Load(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        _skeletonComponent = &_ecs.GetRegistry().emplace<SkeletonComponent>(entity);
        _skeletonComponent->root = entity;

        RecursiveLoadNode(entity, currentNodeIndex, parent);
    }

private:
    ECSModule& _ecs;
    const Hierarchy& _hierarchy;
    entt::entity _animationControlEntity;

    SkeletonComponent* _skeletonComponent;

    void RecursiveLoadNode(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        const Hierarchy::Node& currentNode = _hierarchy.nodes[currentNodeIndex];

        _ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name;
        _ecs.GetRegistry().emplace<AnimationTransformComponent>(entity);
        _ecs.GetRegistry().emplace<HideOrphan>(entity);
        auto& skeletonNode = _ecs.GetRegistry().emplace<SkeletonNodeComponent>(entity);
        SkeletonHelpers::InitializeSkeletonNode(skeletonNode);

        if (parent != entt::null)
        {
            SkeletonHelpers::AttachChild(_ecs.GetRegistry(), parent, entity);
        }

        AnimationTransformHelpers::SetLocalTransform(_ecs.GetRegistry(), entity, currentNode.transform);

        if (!currentNode.animationSplines.empty())
        {
            assert(_animationControlEntity != entt::null);

            auto& animationChannel = _ecs.GetRegistry().emplace<AnimationChannelComponent>(entity);
            animationChannel.animationSplines = currentNode.animationSplines;
            animationChannel.animationControlEntity = _animationControlEntity;
        }

        if (currentNode.joint.has_value())
        {
            auto& joint = _ecs.GetRegistry().emplace<JointSkinDataComponent>(entity);
            joint.inverseBindMatrix = currentNode.joint.value().inverseBind;
            joint.jointIndex = currentNode.joint.value().index;
            joint.skeletonEntity = _skeletonComponent->root;
        }

        for (const auto& nodeIndex : currentNode.children)
        {
            const entt::entity childEntity = _ecs.GetRegistry().create();
            RecursiveLoadNode(childEntity, nodeIndex, entity);
        }
    }
};

entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, const GPUModel& gpuModel, const CPUModel& cpuModel, const Hierarchy& hierarchy, const std::vector<Animation>& animations)
{
    ZoneScopedN("Instantiate Scene");
    entt::entity rootEntity = ecs.GetRegistry().create();

    std::unordered_map<uint32_t, entt::entity> entityLUT;

    entt::entity animationControlEntity = entt::null;
    if (!animations.empty())
    {
        ecs.GetRegistry().emplace<AnimationControlComponent>(rootEntity, animations, std::nullopt, std::nullopt, 0.0f, 0.0f);
        animationControlEntity = rootEntity;
    }

    RecursiveNodeLoader recursiveNodeLoader { ecs, hierarchy, cpuModel, gpuModel, animationControlEntity, entityLUT };
    recursiveNodeLoader.Load(rootEntity, hierarchy.root, entt::null);

    entt::entity skeletonEntity = entt::null;
    if (hierarchy.skeletonRoot.has_value())
    {
        skeletonEntity = ecs.GetRegistry().create();
        ecs.GetRegistry().emplace<TransformComponent>(skeletonEntity);
        ecs.GetRegistry().emplace<RelationshipComponent>(skeletonEntity);

        auto firstChild = ecs.GetRegistry().get<RelationshipComponent>(rootEntity).first;

        RecursiveSkeletonLoader recursiveSkeletonLoader { ecs, hierarchy, animationControlEntity };
        recursiveSkeletonLoader.Load(skeletonEntity, hierarchy.skeletonRoot.value(), entt::null);
        RelationshipHelpers::AttachChild(ecs.GetRegistry(), firstChild, skeletonEntity);
    }

    if (skeletonEntity != entt::null)
    {
        // Links skeleton entities to the skinned mesh components.
        for (size_t i = 0; i < hierarchy.nodes.size(); ++i)
        {
            const Hierarchy::Node& node = hierarchy.nodes[i];
            if (node.skeletonNode.has_value() && node.meshIndex.has_value() && std::get<0>(node.meshIndex.value()) == MeshType::eSKINNED)
            {
                SkinnedMeshComponent& skinnedMeshComponent = ecs.GetRegistry().get<SkinnedMeshComponent>(entityLUT[i]);
                skinnedMeshComponent.skeletonEntity = skeletonEntity;
            }
        }
    }

    return rootEntity;
}

entt::entity SceneLoading::LoadModel(Engine& engine, const std::string& path)
{
    auto& threadPool = engine.GetModule<ThreadModule>().GetPool();
    CPUModel cpuData {};

    {
        ZoneScoped;
        std::string zone = path + " CPU parsing";
        ZoneName(zone.c_str(), 128);

        cpuData = ModelLoading::LoadGLTFFast(threadPool, path);
    }

    auto& rendererModule = engine.GetModule<RendererModule>();
    auto gpuHandle = rendererModule.LoadModels({ cpuData }).front();

    auto& modelResourceManager = rendererModule.GetRenderer()->GetContext()->Resources()->ModelResourceManager();
    const GPUModel& gpuModel = *modelResourceManager.Access(gpuHandle);

    return LoadModelIntoECSAsHierarchy(engine.GetModule<ECSModule>(), gpuModel, cpuData, cpuData.hierarchy, cpuData.animations);
}
