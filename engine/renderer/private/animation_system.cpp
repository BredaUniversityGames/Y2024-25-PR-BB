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

#include <components/name_component.hpp>
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

            bool isSingleAnimation = animationControl.activeAnimation.has_value() && !animationControl.transitionAnimation.has_value();
            bool hasTransitionAnimation = animationControl.activeAnimation.has_value() && animationControl.transitionAnimation.has_value();

            if (isSingleAnimation)
            {
                Animation& currentAnimation = animationControl.animations[animationControl.activeAnimation.value()];
                currentAnimation.Update(dt / 1000.0f);
            }
            else if (hasTransitionAnimation)
            {
                Animation& activeAnimation = animationControl.animations[animationControl.activeAnimation.value()];
                Animation& transitionAnimation = animationControl.animations[animationControl.transitionAnimation.value()];
                bool equalAnimations = animationControl.activeAnimation.value() == animationControl.transitionAnimation.value();
                float timeStep = dt / 1000.0f;
                float blendWeight = 1.0 - animationControl.remainingBlendTime / animationControl.blendTime;

                animationControl.remainingBlendTime -= timeStep;
                if (animationControl.remainingBlendTime <= 0.0f)
                {
                    animationControl.remainingBlendTime = 0.0;
                    animationControl.blendTime = 0.0;
                    animationControl.transitionAnimation = std::nullopt;
                }

                float durationA = activeAnimation.duration;
                float durationB = transitionAnimation.duration;

                // Compute the time scaling factors
                float factorA = (1.0f - blendWeight) + blendWeight * (durationA / durationB);
                float factorB = blendWeight + (1.0f - blendWeight) * (durationB / durationA);

                // Update times
                activeAnimation.time += timeStep / factorB;
                transitionAnimation.time += equalAnimations ? 0.0f : timeStep / factorA;

                if (activeAnimation.time > activeAnimation.duration && activeAnimation.looping)
                {
                    activeAnimation.time = std::fmod(activeAnimation.time, activeAnimation.duration);
                }

                if (transitionAnimation.time > transitionAnimation.duration && transitionAnimation.looping)
                {
                    transitionAnimation.time = std::fmod(transitionAnimation.time, transitionAnimation.duration);
                }
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
                float blendWeight = 1.0 - animationControl->remainingBlendTime / animationControl->blendTime;

                transform.position = glm::mix(transitionTransform.value().position, activeTransform.position, blendWeight);
                transform.scale = glm::mix(transitionTransform.value().scale, activeTransform.scale, blendWeight);
                transform.rotation = glm::slerp(transitionTransform.value().rotation, activeTransform.rotation, blendWeight);
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
    const auto& view = ecs.GetRegistry().view<SkeletonNodeComponent, AnimationTransformComponent, NameComponent>();

    auto [node, transform, name] = view[entity];

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