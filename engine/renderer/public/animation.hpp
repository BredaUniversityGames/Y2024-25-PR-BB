#pragma once

#include <algorithm>
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
            time = std::fmod(time, duration);
        }
    }
};

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
