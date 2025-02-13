#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "resources/animation.hpp"

struct AnimationControlComponent
{
    std::vector<Animation> animations;
    std::optional<uint32_t> activeAnimation { std::nullopt };

    void PlayByIndex(uint32_t animationIndex, float speed = 1.0f, bool looping = false);
    void Play(const std::string& name, float speed = 1.0f, bool looping = false);
    void Stop();
    void Pause();
    void Resume();
    Animation::PlaybackOptions CurrentPlayback();
    std::optional<std::string> CurrentAnimationName();
    std::optional<uint32_t> CurrentAnimationIndex();
    bool AnimationFinished();
};

struct AnimationChannelComponent
{
    AnimationControlComponent* animationControl { nullptr };
    // Keys are based on the animation index from the AnimationControl.
    std::unordered_map<uint32_t, TransformAnimationSpline> animationSplines;
};
