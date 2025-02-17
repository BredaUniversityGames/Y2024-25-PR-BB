#include "animation_system.hpp"

#include "animation.hpp"
#include "components/animation_channel_component.hpp"
#include "components/animation_transform_component.hpp"
#include "components/joint_component.hpp"
#include "components/relationship_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "passes/debug_pass.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"

#include <glm/gtx/quaternion.hpp>
#include <tracy/Tracy.hpp>

AnimationSystem::AnimationSystem(RendererModule& rendererModule)
    : _rendererModule(rendererModule)
{
}

AnimationSystem::~AnimationSystem() = default;

void AnimationSystem::Update(ECSModule& ecs, float dt)
{
    {
        ZoneScopedN("Tick animations");
        const auto animationControlView = ecs.GetRegistry().view<AnimationControlComponent>();
        for (auto entity : animationControlView)
        {
            auto& animationControl = animationControlView.get<AnimationControlComponent>(entity);

            if (animationControl.activeAnimation.has_value())
            {
                Animation& currentAnimation = animationControl.animations[animationControl.activeAnimation.value()];
                currentAnimation.Update(dt / 1000.0f);
            }
        }
    }

    {
        ZoneScopedN("Animate Transforms");
        const auto animationView = ecs.GetRegistry().view<AnimationTransformComponent, AnimationChannelComponent>();
        for (auto entity : animationView)
        {
            auto& animationChannel = animationView.get<AnimationChannelComponent>(entity);
            auto* animationControl = animationChannel.animationControl;
            if (animationControl->activeAnimation.has_value())
            {
                auto& transform = animationView.get<AnimationTransformComponent>(entity);
                auto& activeAnimation = animationChannel.animationSplines[animationControl->activeAnimation.value()];
                float time = animationControl->animations[animationControl->activeAnimation.value()].time;
                if (activeAnimation.translation.has_value())
                {
                    glm::vec3 position = activeAnimation.translation.value().Sample(time);

                    transform.position = position;
                }
                if (activeAnimation.rotation.has_value())
                {
                    glm::quat rotation = activeAnimation.rotation.value().Sample(time);

                    transform.rotation = rotation;
                }
                if (activeAnimation.scaling.has_value())
                {
                    glm::vec3 scale = activeAnimation.scaling.value().Sample(time);

                    transform.scale = scale;
                }
            }
        }
    }

    {
        ZoneScopedN("Calculate World Matrix");
        const auto skeletonView = ecs.GetRegistry().view<SkeletonComponent, WorldMatrixComponent>();
        for (auto entity : skeletonView)
        {
            const auto& skeleton = skeletonView.get<SkeletonComponent>(entity);
            const auto& matrix = skeletonView.get<WorldMatrixComponent>(entity);
            TraverseAndCalculateMatrix(skeleton.root, TransformHelpers::GetWorldMatrix(matrix), ecs, skeleton);
        }
    }
}

void AnimationSystem::Render(const ECSModule& ecs) const
{
    // Draw skeletons as debug lines
    const auto debugView = ecs.GetRegistry().view<const JointComponent, const SkeletonNodeComponent, const SkeletonMatrixTransform>();
    for (auto entity : debugView)
    {
        const auto& node = debugView.get<SkeletonNodeComponent>(entity);
        const auto& transform = debugView.get<SkeletonMatrixTransform>(entity);

        if (node.parent != entt::null)
        {
            const auto& parentTransform = debugView.get<SkeletonMatrixTransform>(node.parent);

            glm::vec3 position { transform.world[3][0], transform.world[3][1], transform.world[3][2] };
            glm::vec3 parentPosition { parentTransform.world[3][0], parentTransform.world[3][1], parentTransform.world[3][2] };

            _rendererModule.GetRenderer()->GetDebugPipeline().AddLine(position, parentPosition);
        }
    }
}

void AnimationSystem::Inspect()
{
}

void AnimationSystem::TraverseAndCalculateMatrix(entt::entity entity, const glm::mat4& parentMatrix, ECSModule& ecs, const SkeletonComponent& skeleton)
{
    const auto& view = ecs.GetRegistry().view<SkeletonNodeComponent, AnimationTransformComponent>();

    auto [node, transform] = view[entity];

    auto& matrix = ecs.GetRegistry().get_or_emplace<SkeletonMatrixTransform>(entity);

    const glm::mat4 translationMatrix = glm::translate(glm::mat4 { 1.0f }, transform.position);
    const glm::mat4 rotationMatrix = glm::toMat4(transform.rotation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4 { 1.0f }, transform.scale);

    matrix.world = parentMatrix * translationMatrix * rotationMatrix * scaleMatrix;

    for (size_t i = 0; i < node.children.size(); ++i)
    {
        if (node.children[i] == entt::null)
        {
            break;
        }

        TraverseAndCalculateMatrix(node.children[i], matrix.world, ecs, skeleton);
    }
}