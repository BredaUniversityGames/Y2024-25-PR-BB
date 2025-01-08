#include "animation_system.hpp"

#include "animation.hpp"
#include "components/animation_channel_component.hpp"
#include "components/joint_component.hpp"
#include "components/relationship_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "passes/debug_pass.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"

#include <tracy/Tracy.hpp>

AnimationSystem::AnimationSystem(RendererModule& rendererModule)
    : _rendererModule(rendererModule)
{
}

AnimationSystem::~AnimationSystem() = default;

void AnimationSystem::Update(ECSModule& ecs, float dt)
{
    ZoneScoped;

    const auto animationControlView = ecs.GetRegistry().view<AnimationControlComponent>();
    for (auto entity : animationControlView)
    {
        auto& animationControl = animationControlView.get<AnimationControlComponent>(entity);

        if (animationControl.activeAnimation.has_value())
        {
            Animation& currentAnimation = animationControl.animations[animationControl.activeAnimation.value()];
            currentAnimation.Update(dt / 1000.0f); // TODO: Frame index might not be needed anymore.
        }
    }

    const auto view = ecs.GetRegistry().view<TransformComponent, AnimationChannelComponent>();
    for (auto entity : view)
    {
        auto& animationChannel = view.get<AnimationChannelComponent>(entity);
        auto* animationControl = animationChannel.animationControl;
        if (animationControl->activeAnimation.has_value())
        {
            auto& activeAnimation = animationChannel.animationSplines[animationControl->activeAnimation.value()];
            float time = animationControl->animations[animationControl->activeAnimation.value()].time;
            if (activeAnimation.translation.has_value())
            {
                glm::vec3 position = activeAnimation.translation.value().Sample(time);

                TransformHelpers::SetLocalPosition(ecs.GetRegistry(), entity, position);
            }
            if (activeAnimation.rotation.has_value())
            {
                glm::quat rotation = activeAnimation.rotation.value().Sample(time);

                TransformHelpers::SetLocalRotation(ecs.GetRegistry(), entity, rotation);
            }
            if (activeAnimation.scaling.has_value())
            {
                glm::vec3 scale = activeAnimation.scaling.value().Sample(time);

                TransformHelpers::SetLocalScale(ecs.GetRegistry(), entity, scale);
            }
        }
    }
}

void AnimationSystem::Render(const ECSModule& ecs) const
{
    ZoneScoped;
    // Draw skeletons as debug lines
    const auto debugView = ecs.GetRegistry().view<const JointComponent, const RelationshipComponent, const WorldMatrixComponent>();
    for (auto entity : debugView)
    {
        const auto& relationship = debugView.get<RelationshipComponent>(entity);
        const auto& transform = debugView.get<WorldMatrixComponent>(entity);

        glm::mat4 matrix = TransformHelpers::GetWorldMatrix(transform);
        glm::mat4 parentMatrix = TransformHelpers::GetWorldMatrix(ecs.GetRegistry(), relationship.parent);

        glm::vec3 position { matrix[3][0], matrix[3][1], matrix[3][2] };
        glm::vec3 parentPosition { parentMatrix[3][0], parentMatrix[3][1], parentMatrix[3][2] };

        _rendererModule.GetRenderer()->GetDebugPipeline().AddLine(position, parentPosition);
    }
}

void AnimationSystem::Inspect()
{
}