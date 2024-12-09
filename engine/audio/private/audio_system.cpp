#include "audio_system.hpp"

#include "glm/glm.hpp"

#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

AudioSystem::AudioSystem(ECSModule& ecs, AudioModule& audioModule)
    : _ecs(ecs)
    , _audioModule(audioModule)
{
}
void AudioSystem::Update(ECSModule& ecs, float dt)
{
    {
        const auto& view = ecs.GetRegistry().view<AudioListenerComponent>();

        if (!view.empty())
        {
            const auto listener = view.front();
            auto* transform = ecs.GetRegistry().try_get<TransformComponent>(listener);
            if (transform)
            {
                _audioModule.SetListener3DAttributes(TransformHelpers::GetWorldPosition(ecs.GetRegistry(), listener));
            }
        }
    }

    {
        const auto& view = ecs.GetRegistry().view<AudioEmitterComponent, TransformComponent>();
        for (const auto e : view)
        {
            AudioEmitterComponent& emitter = ecs.GetRegistry().get<AudioEmitterComponent>(e);
            TransformComponent& transform = ecs.GetRegistry().get<TransformComponent>(e);

            for (auto id : emitter.ids)
            {
            }
        }
    }
}
void AudioSystem::Inspect()
{
}