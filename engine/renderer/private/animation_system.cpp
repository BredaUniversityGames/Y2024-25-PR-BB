#include "animation_system.hpp"

#include "animation.hpp"
#include "components/animation_channel_component.hpp"
#include "components/joint_component.hpp"
#include "components/relationship_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "ecs_module.hpp"
#include "pipelines/debug_pipeline.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"

#include <tracy/Tracy.hpp>

AnimationSystem::AnimationSystem(RendererModule& rendererModule)
    : _rendererModule(rendererModule)
    , _frameIndex(0)
{
}

AnimationSystem::~AnimationSystem() = default;

void AnimationSystem::Update(ECSModule& ecs, float dt)
{
    ZoneScoped;
    const auto view = ecs.GetRegistry().view<TransformComponent, AnimationChannelComponent>();
    for (auto entity : view)
    {
        auto& animation = view.get<AnimationChannelComponent>(entity);

        animation.animation->Update(dt / 1000.0f, _frameIndex);

        if (animation.translation.has_value())
        {
            glm::vec3 position = animation.translation.value().Sample(animation.animation->time);

            TransformHelpers::SetLocalPosition(ecs.GetRegistry(), entity, position);
        }
        if (animation.rotation.has_value())
        {
            glm::quat rotation = animation.rotation.value().Sample(animation.animation->time);

            TransformHelpers::SetLocalRotation(ecs.GetRegistry(), entity, rotation);
        }
        if (animation.scaling.has_value())
        {
            glm::vec3 scale = animation.scaling.value().Sample(animation.animation->time);

            TransformHelpers::SetLocalScale(ecs.GetRegistry(), entity, scale);
        }
    }

    ++_frameIndex;
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