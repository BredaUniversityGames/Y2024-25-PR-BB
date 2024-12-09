#include "audio_system.hpp"
#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs.hpp"

AudioSystem::AudioSystem(ECS& ecs, AudioModule& audioModule)
    : _ecs(ecs)
    , _audioModule(audioModule)
{
}
void AudioSystem::Update(ECS& ecs, float dt)
{
    // auto& listeners = ecs.registry.view<AudioListenerComponent>().;

    const auto& view = ecs.registry.view<AudioListenerComponent>();

    if (!view.empty())
    {
        const auto listener = view.front();
        auto* transform = ecs.registry.try_get<TransformComponent>(listener);
        if (transform)
        {
            _audioModule.SetListener3DAttributes(TransformHelpers::GetWorldPosition(ecs.registry, listener));
        }
    }

    // const auto& view = ecs.registry.view<AudioEmitterComponent, TransformComponent>();
    // for (const auto e : view)
    // {
    //     AudioEmitterComponent& emitter = ecs.registry.get<AudioEmitterComponent>(e);
    //     TransformComponent& transform = ecs.registry.get<TransformComponent>(e);
    //
    //
    // }
}
void AudioSystem::Render(const ECS& ecs) const
{
}
void AudioSystem::Inspect()
{
}