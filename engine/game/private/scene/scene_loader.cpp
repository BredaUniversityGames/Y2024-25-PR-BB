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
    RecursiveNodeLoader(ECSModule& ecs, PhysicsModule& physics, const Hierarchy& hierarchy, const CPUModel& cpuModel, const GPUModel& gpuModel, AnimationControlComponent* animationControl, std::unordered_map<uint32_t, entt::entity>& entityLut)
        : _ecs(ecs)
        , _physics(physics)
        , _hierarchy(hierarchy)
        , _cpuModel(cpuModel)
        , _gpuModel(gpuModel)
        , _animationControl(animationControl)
        , _entityLUT(entityLut)
    {
    }

    void Load(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        const Node& currentNode = _hierarchy.nodes[currentNodeIndex];

        _entityLUT[currentNodeIndex] = entity;

        _ecs.GetRegistry().emplace<NameComponent>(entity).name = currentNode.name;
        _ecs.GetRegistry().emplace<TransformComponent>(entity);

        _ecs.GetRegistry().emplace<RelationshipComponent>(entity);
        if (parent != entt::null)
        {
            RelationshipHelpers::AttachChild(_ecs.GetRegistry(), parent, entity);
        }

        TransformHelpers::SetLocalTransform(_ecs.GetRegistry(), entity, currentNode.transform);

        if (currentNode.mesh.has_value())
        {
            auto index = currentNode.mesh.value().index;
            switch (currentNode.mesh.value().type)
            {
            case MeshType::eSTATIC:
            {
                _ecs.GetRegistry().emplace<StaticMeshComponent>(entity).mesh = _gpuModel.staticMeshes.at(index);
                _ecs.GetRegistry().emplace<IsStaticDraw>(entity);

                // check if it should have collider

                auto rb = RigidbodyComponent(_physics.GetBodyInterface(), _cpuModel.colliders.at(index), false);
                _ecs.GetRegistry().emplace<RigidbodyComponent>(entity, std::move(rb));

                break;
            }
            case MeshType::eSKINNED:
                _ecs.GetRegistry().emplace<SkinnedMeshComponent>(entity).mesh = _gpuModel.skinnedMeshes.at(index);
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

        if (currentNode.light.has_value())
        {
            switch (currentNode.light->type)
            {
            case NodeLightType::Directional:
            {
                auto& directionalLight = _ecs.GetRegistry().emplace<DirectionalLightComponent>(entity);
                directionalLight.color = currentNode.light->color * currentNode.light->intensity;
                break;
            }
            case NodeLightType::Point:
            {
                auto& pointLight = _ecs.GetRegistry().emplace<PointLightComponent>(entity);
                pointLight.color = currentNode.light->color;
                pointLight.intensity = currentNode.light->intensity;
                pointLight.range = currentNode.light->range;
                break;
            }
            case NodeLightType::Spot:
            {
                assert(!"Spot lights are not supported!");
                break;
            }
            }
        }

        for (const auto& nodeIndex : currentNode.childrenIndices)
        {
            const entt::entity childEntity = _ecs.GetRegistry().create();
            Load(childEntity, nodeIndex, entity);
        }
    }

private:
    ECSModule& _ecs;
    PhysicsModule& _physics;
    const Hierarchy& _hierarchy;
    const CPUModel& _cpuModel;
    const GPUModel& _gpuModel;
    AnimationControlComponent* _animationControl;
    std::unordered_map<uint32_t, entt::entity>& _entityLUT;
};

class RecursiveSkeletonLoader
{
public:
    RecursiveSkeletonLoader(ECSModule& ecs, const Hierarchy& hierarchy, AnimationControlComponent* animationControl)
        : _ecs(ecs)
        , _hierarchy(hierarchy)
        , _animationControl(animationControl)
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
    AnimationControlComponent* _animationControl;

    SkeletonComponent* _skeletonComponent;

    void RecursiveLoadNode(entt::entity entity, uint32_t currentNodeIndex, entt::entity parent)
    {
        const Node& currentNode = _hierarchy.nodes[currentNodeIndex];

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
            assert(_animationControl != nullptr);

            auto& animationChannel = _ecs.GetRegistry().emplace<AnimationChannelComponent>(entity);
            animationChannel.animationSplines = currentNode.animationSplines;
            animationChannel.animationControl = _animationControl;
        }

        if (currentNode.joint.has_value())
        {
            auto& joint = _ecs.GetRegistry().emplace<JointSkinDataComponent>(entity);
            joint.inverseBindMatrix = currentNode.joint.value().inverseBind;
            joint.jointIndex = currentNode.joint.value().index;
            joint.skeletonEntity = _skeletonComponent->root;
        }

        for (const auto& nodeIndex : currentNode.childrenIndices)
        {
            const entt::entity childEntity = _ecs.GetRegistry().create();
            RecursiveLoadNode(childEntity, nodeIndex, entity);
        }
    }
};

entt::entity LoadModelIntoECSAsHierarchy(ECSModule& ecs, PhysicsModule& physics, const GPUModel& gpuModel, const CPUModel& cpuModel)
{
    ZoneScopedN("Instantiate Scene");
    entt::entity rootEntity = ecs.GetRegistry().create();

    std::unordered_map<uint32_t, entt::entity> entityLUT;

    AnimationControlComponent* animationControl = nullptr;
    if (!cpuModel.animations.empty())
    {
        animationControl = &ecs.GetRegistry().emplace<AnimationControlComponent>(rootEntity, cpuModel.animations, std::nullopt);
    }

    RecursiveNodeLoader recursiveNodeLoader { ecs, physics, cpuModel.hierarchy, cpuModel, gpuModel, animationControl, entityLUT };
    recursiveNodeLoader.Load(rootEntity, cpuModel.hierarchy.root, entt::null);

    entt::entity skeletonEntity = entt::null;
    if (cpuModel.hierarchy.skeletonRoot.has_value())
    {
        skeletonEntity = ecs.GetRegistry().create();
        ecs.GetRegistry().emplace<TransformComponent>(skeletonEntity);
        ecs.GetRegistry().emplace<RelationshipComponent>(skeletonEntity);

        auto firstChild = ecs.GetRegistry().get<RelationshipComponent>(rootEntity).first;

        RecursiveSkeletonLoader recursiveSkeletonLoader { ecs, cpuModel.hierarchy, animationControl };
        recursiveSkeletonLoader.Load(skeletonEntity, cpuModel.hierarchy.skeletonRoot.value(), entt::null);
        RelationshipHelpers::AttachChild(ecs.GetRegistry(), firstChild, skeletonEntity);
    }

    if (skeletonEntity != entt::null)
    {
        // Links skeleton entities to the skinned mesh components.
        for (size_t i = 0; i < cpuModel.hierarchy.nodes.size(); ++i)
        {
            const Node& node = cpuModel.hierarchy.nodes[i];
            if (node.skeletonNode.has_value() && node.mesh.has_value() && node.mesh->type == MeshType::eSKINNED)
            {
                SkinnedMeshComponent& skinnedMeshComponent = ecs.GetRegistry().get<SkinnedMeshComponent>(entityLUT[i]);
                skinnedMeshComponent.skeletonEntity = skeletonEntity;
            }
        }
    }

    return rootEntity;
}

std::shared_ptr<SceneData> SceneLoader::LoadModel(Engine& engine, std::string_view path)
{
    if (auto it = _models.find(std::string(path)); it != _models.end())
    {
        return it->second;
    }

    auto& threadPool = engine.GetModule<ThreadModule>().GetPool();
    CPUModel cpuData {};

    {
        ZoneScoped;
        std::string zone = std::string(path) + " CPU parsing";
        ZoneName(zone.c_str(), 128);

        cpuData = ModelLoading::LoadGLTFFast(threadPool, path);
    }

    auto& rendererModule = engine.GetModule<RendererModule>();
    auto gpuHandle = rendererModule.LoadModels({ cpuData }).front();

    auto ret = std::make_shared<SceneData>(std::move(cpuData), gpuHandle);
    _models[std::string(path)] = ret;

    return ret;
}

entt::entity SceneData::Instantiate(Engine& engine)
{
    auto& modelResourceManager = engine.GetModule<RendererModule>().GetRenderer()->GetContext()->Resources()->ModelResourceManager();
    const GPUModel& model = *modelResourceManager.Access(gpuModel);

    return LoadModelIntoECSAsHierarchy(
        engine.GetModule<ECSModule>(),
        engine.GetModule<PhysicsModule>(),
        model,
        cpuModel);
}