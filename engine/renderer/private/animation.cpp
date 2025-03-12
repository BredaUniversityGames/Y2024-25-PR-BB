#include "animation.hpp"

#include <algorithm>

#include "log.hpp"

void AnimationControlComponent::PlayByIndex(uint32_t animationIndex, float speed, bool looping)
{
    activeAnimation = animationIndex;
    animations[animationIndex].time = 0.0f;
    animations[animationIndex].looping = looping;
    animations[animationIndex].speed = speed;
    animations[animationIndex].playbackOption = Animation::PlaybackOptions::ePlaying;
}

void AnimationControlComponent::Play(const std::string& name, float speed, bool looping)
{
    auto it = std::find_if(animations.begin(), animations.end(), [&name](const auto& anim)
        { return anim.name == name; });

    if (it != animations.end())
    {
        PlayByIndex(std::distance(animations.begin(), it), speed, looping);
    }
    else
    {
        bblog::warn("Tried to use invalid animation name: {}", name);
    }
}

void AnimationControlComponent::Transition(uint32_t source, uint32_t target, float ratio, float speed, bool looping)
{
    transitionAnimation = source;
    animations[source].time = 0.0f;
    animations[source].looping = looping;
    animations[source].speed = speed;
    animations[source].playbackOption = Animation::PlaybackOptions::ePlaying;

    activeAnimation = target;
    animations[target].time = 0.0f;
    animations[target].looping = looping;
    animations[target].speed = speed;
    animations[target].playbackOption = Animation::PlaybackOptions::ePlaying;

    blendRatio = ratio;
}

void AnimationControlComponent::Stop()
{
    if (!activeAnimation.has_value())
    {
        return;
    }
    Animation& animation = animations[activeAnimation.value()];
    animation.time = 0.0f;
    animation.playbackOption = Animation::PlaybackOptions::eStopped;
}

void AnimationControlComponent::Pause()
{
    if (!activeAnimation.has_value())
    {
        return;
    }
    Animation& animation = animations[activeAnimation.value()];
    animation.playbackOption = Animation::PlaybackOptions::ePaused;
}

void AnimationControlComponent::Resume()
{
    if (!activeAnimation.has_value())
    {
        return;
    }
    Animation& animation = animations[activeAnimation.value()];
    animation.playbackOption = Animation::PlaybackOptions::ePlaying;
}

Animation::PlaybackOptions AnimationControlComponent::CurrentPlayback()
{
    if (!activeAnimation.has_value())
    {
        return Animation::PlaybackOptions::eStopped;
    }

    return animations[activeAnimation.value()].playbackOption;
}

std::optional<std::string> AnimationControlComponent::CurrentAnimationName()
{
    if (!activeAnimation.has_value())
    {
        return std::nullopt;
    }

    return animations[activeAnimation.value()].name;
}

std::optional<uint32_t> AnimationControlComponent::CurrentAnimationIndex()
{
    return activeAnimation;
}

bool AnimationControlComponent::AnimationFinished()
{
    if (!activeAnimation.has_value())
    {
        return true;
    }

    return animations[activeAnimation.value()].time > animations[activeAnimation.value()].duration || animations[activeAnimation.value()].playbackOption == Animation::PlaybackOptions::eStopped;
}
