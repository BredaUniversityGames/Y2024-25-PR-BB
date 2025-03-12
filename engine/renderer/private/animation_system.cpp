#include "animation_system.hpp"

#include "animation.hpp"
#include "components/animation_channel_component.hpp"
#include "components/animation_transform_component.hpp"
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

            if (animationControl.activeAnimation.has_value() && !animationControl.transitionAnimation.has_value())
            {
                Animation& currentAnimation = animationControl.animations[animationControl.activeAnimation.value()];
                currentAnimation.Update(dt / 1000.0f);
            }
            else if (animationControl.activeAnimation.has_value() && animationControl.transitionAnimation.has_value())
            {
                Animation& activeAnimation = animationControl.animations[animationControl.activeAnimation.value()];
                Animation& transitionAnimation = animationControl.animations[animationControl.transitionAnimation.value()];

                activeAnimation.Update(dt / 1000.0f);

                float scaledTime = activeAnimation.time * (transitionAnimation.duration / activeAnimation.duration);
                transitionAnimation.time = scaledTime;
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
            auto& transform = animationView.get<AnimationTransformComponent>(entity);

            AnimationTransformComponent activeTransform = transform;
            std::optional<AnimationTransformComponent> transitionTransform = std::nullopt;

            if (animationControl->activeAnimation.has_value())
            {
                auto& activeAnimation = animationChannel.animationSplines[animationControl->activeAnimation.value()];
                float time = animationControl->animations[animationControl->activeAnimation.value()].time;

                if (activeAnimation.translation.has_value())
                {
                    activeTransform.position = activeAnimation.translation.value().Sample(time);
                }
                if (activeAnimation.rotation.has_value())
                {
                    activeTransform.rotation = activeAnimation.rotation.value().Sample(time);
                }
                if (activeAnimation.scaling.has_value())
                {
                    activeTransform.scale = activeAnimation.scaling.value().Sample(time);
                }
            }

            if (animationControl->transitionAnimation.has_value())
            {
                transitionTransform = transform;

                auto& transitionAnimation = animationChannel.animationSplines[animationControl->transitionAnimation.value()];
                float time = animationControl->animations[animationControl->transitionAnimation.value()].time;

                if (transitionAnimation.translation.has_value())
                {
                    transitionTransform.value().position = transitionAnimation.translation.value().Sample(time);
                }
                if (transitionAnimation.rotation.has_value())
                {
                    transitionTransform.value().rotation = transitionAnimation.rotation.value().Sample(time);
                }
                if (transitionAnimation.scaling.has_value())
                {
                    transitionTransform.value().scale = transitionAnimation.scaling.value().Sample(time);
                }
            }

            if (transitionTransform.has_value())
            {
                transform.position = glm::mix(transitionTransform.value().position, activeTransform.position, animationControl->blendRatio);
                transform.scale = glm::mix(transitionTransform.value().scale, activeTransform.scale, animationControl->blendRatio);
                transform.rotation = glm::slerp(transitionTransform.value().rotation, activeTransform.rotation, animationControl->blendRatio);
            }
            else
            {
                transform = activeTransform;
            }
        }
    }

    {
        ZoneScopedN("Calculate World Matrix");
        const auto skeletonView = ecs.GetRegistry().view<SkeletonComponent>();
        for (auto entity : skeletonView)
        {
            const auto& skeleton = skeletonView.get<SkeletonComponent>(entity);
            RecursiveCalculateMatrix(skeleton.root, glm::identity<glm::mat4>(), ecs, skeleton);
        }
    }
}

void AnimationSystem::Render(const ECSModule& ecs) const
{
    // Draw skeletons as debug lines
    const auto debugView = ecs.GetRegistry().view<const JointSkinDataComponent, const SkeletonNodeComponent, const JointWorldTransformComponent>();
    for (auto entity : debugView)
    {
        const auto& node = debugView.get<SkeletonNodeComponent>(entity);
        const auto& transform = debugView.get<JointWorldTransformComponent>(entity);

        if (node.parent != entt::null)
        {
            const auto& parentTransform = debugView.get<JointWorldTransformComponent>(node.parent);

            glm::vec3 position { transform.world[3][0], transform.world[3][1], transform.world[3][2] };
            glm::vec3 parentPosition { parentTransform.world[3][0], parentTransform.world[3][1], parentTransform.world[3][2] };

            _rendererModule.GetRenderer()->GetDebugPipeline().AddLine(position, parentPosition);
        }
    }
}

void AnimationSystem::Inspect()
{
}

void AnimationSystem::RecursiveCalculateMatrix(entt::entity entity, const glm::mat4& parentMatrix, ECSModule& ecs, const SkeletonComponent& skeleton)
{
    const auto& view = ecs.GetRegistry().view<SkeletonNodeComponent, AnimationTransformComponent>();

    auto [node, transform] = view[entity];

    auto& matrix = ecs.GetRegistry().get_or_emplace<JointWorldTransformComponent>(entity);

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

        RecursiveCalculateMatrix(node.children[i], matrix.world, ecs, skeleton);
    }
}