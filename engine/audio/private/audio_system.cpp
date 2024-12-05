#include "audio_system.hpp"
#include "audio_module.hpp"

AudioSystem::AudioSystem(ECS& ecs, AudioModule& audioModule)
    : _ecs(ecs)
    , _audioModule(audioModule)
{
}
void AudioSystem::Update(ECS& ecs, float dt)
{
}
void AudioSystem::Render(const ECS& ecs) const
{
}
void AudioSystem::Inspect()
{
}