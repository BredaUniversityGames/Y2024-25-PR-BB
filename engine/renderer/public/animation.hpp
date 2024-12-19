#pragma once

#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include <vector>

using Translation = glm::vec3;
using Rotation = glm::quat;
using Scale = glm::vec3;

template <typename T>
struct AnimationSpline
{
    T Sample(float time)
    {
        auto it = std::lower_bound(timestamps.begin(), timestamps.end(), time);

        if (it == timestamps.begin())
        {
            return values.front();
        }

        if (it == timestamps.end())
        {
            return values.back();
        }

        uint32_t i = it - timestamps.begin();

        float t = (time - timestamps[i - 1]) / (timestamps[i] - timestamps[i - 1]);

        return glm::mix(values[i - 1], values[i], t);
    }

    std::vector<float> timestamps;
    std::vector<T> values;
};

struct Animation
{
    enum class PlaybackOptions
    {
        ePlaying,
        ePaused,
        eStopped,
    };

    std::string name { "" };
    float duration { 0.0f };
    float time { 0.0f };
    float speed { 1.0f };
    PlaybackOptions playbackOption { Animation::PlaybackOptions::eStopped };
    bool looping = false;

    void Update(float dt)
    {
        switch (playbackOption)
        {
        case PlaybackOptions::ePlaying:
            time += dt * speed;
            break;
        case PlaybackOptions::ePaused:
            // Do nothing.
            break;
        case PlaybackOptions::eStopped:
            time = 0.0f;
            break;
        }

        if (time > duration && looping)
        {
            time = 0.0f;
        }
    }
};

struct AnimationControlComponent
{
    std::vector<Animation> animations;
    std::optional<uint32_t> activeAnimation { std::nullopt };

    void PlayByIndex(uint32_t animationIndex, float speed = 1.0f, bool looping = false)
    {
        activeAnimation = animationIndex;
        animations[animationIndex].time = 0.0f;
        animations[animationIndex].looping = looping;
        animations[animationIndex].speed = speed;
        animations[animationIndex].playbackOption = Animation::PlaybackOptions::ePlaying;
    }
    void Play(const std::string& name, float speed = 1.0f, bool looping = false)
    {
        auto it = std::find_if(animations.begin(), animations.end(), [&name](const auto& anim)
            { return anim.name == name; });

        if (it != animations.end())
        {
            PlayByIndex(std::distance(animations.begin(), it), speed, looping);
        }
    }
    void Stop()
    {
        if (!activeAnimation.has_value())
        {
            return;
        }
        Animation& animation = animations[activeAnimation.value()];
        animation.time = 0.0f;
        animation.playbackOption = Animation::PlaybackOptions::eStopped;
    }

    void Pause()
    {
        if (!activeAnimation.has_value())
        {
            return;
        }
        Animation& animation = animations[activeAnimation.value()];
        animation.playbackOption = Animation::PlaybackOptions::ePaused;
    }
    void Resume()
    {
        if (!activeAnimation.has_value())
        {
            return;
        }
        Animation& animation = animations[activeAnimation.value()];
        animation.playbackOption = Animation::PlaybackOptions::ePlaying;
    }
    Animation::PlaybackOptions CurrentPlayback()
    {
        if (!activeAnimation.has_value())
        {
            return Animation::PlaybackOptions::eStopped;
        }

        return animations[activeAnimation.value()].playbackOption;
    }
    std::optional<std::string> CurrentAnimationName()
    {
        if (!activeAnimation.has_value())
        {
            return std::nullopt;
        }

        return animations[activeAnimation.value()].name;
    }
    std::optional<uint32_t> CurrentAnimationIndex()
    {
        if (!activeAnimation.has_value())
        {
            return std::nullopt;
        }

        return activeAnimation.value();
    }
    bool AnimationFinished()
    {
        if (!activeAnimation.has_value())
        {
            return true;
        }

        return animations[activeAnimation.value()].time > animations[activeAnimation.value()].duration || animations[activeAnimation.value()].playbackOption == Animation::PlaybackOptions::eStopped;
    }
};
